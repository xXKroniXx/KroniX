DiagnosedVehicles = {} -- [netId] = { time = GetGameTimer(), engine = 0..1000, body = 0..1000 }

local function isVehicleDamaged(vehicle)
    local engine = GetVehicleEngineHealth(vehicle)
    local body = GetVehicleBodyHealth(vehicle)
    return engine < 990.0 or body < 990.0
end

local function runDiagnostics(vehicle)
    if not IsPlayerOnDuty() then
        lib.notify({ description = _U('not_on_duty'), type = 'error' })
        return
    end

    if not isVehicleDamaged(vehicle) then
        lib.notify({ description = _U('vehicle_not_damaged'), type = 'inform' })
        return
    end

    if Config.Diagnostics.itemRequired then
        local hasTool = lib.callback.await('kronix-mechanic:server:hasItem', false, Config.Items.diagnosticTool, 1)
        if not hasTool then
            lib.notify({ description = _U('missing_diagnostic_tool'), type = 'error' })
            return
        end
    end

    local ped = PlayerPedId()
    local success = lib.progressBar({
        duration = Config.Diagnostics.duration,
        label = _U('diagnosing'),
        useWhileDead = false,
        canCancel = true,
        disable = { move = true, car = true, combat = true },
        anim = { dict = 'mini@repair', clip = 'fixing_a_ped' },
    })

    if not success then
        lib.notify({ description = _U('repair_cancelled'), type = 'error' })
        return
    end

    local skillOk = lib.skillCheck({ 'easy', 'medium' })
    if not skillOk then
        lib.notify({ description = _U('diagnostic_failed'), type = 'error' })
        return
    end

    local engine = GetVehicleEngineHealth(vehicle)
    local body = GetVehicleBodyHealth(vehicle)
    local netId = NetworkGetNetworkIdFromEntity(vehicle)

    DiagnosedVehicles[netId] = {
        time = GetGameTimer(),
        engine = engine,
        body = body,
    }

    local kits = CalculateRequiredKits(engine, body)

    lib.notify({
        description = _U('diagnostic_success', math.floor(engine / 10), math.floor(body / 10), kits),
        type = 'success',
        duration = 8000,
    })
end

function CalculateRequiredKits(engine, body)
    local engineMissing = math.max(0, 1000.0 - engine)
    local bodyMissing = math.max(0, 1000.0 - body)

    local kits = math.ceil(engineMissing / Config.Repair.kitsPerEngineDamage) + math.ceil(bodyMissing / Config.Repair.kitsPerBodyDamage)
    return math.max(Config.Repair.minKits, kits)
end

function IsDiagnosisValid(netId)
    local data = DiagnosedVehicles[netId]
    if not data then return false end
    return (GetGameTimer() - data.time) <= Config.Diagnostics.validForMs
end

CreateThread(function()
    exports.ox_target:addGlobalVehicle({
        {
            name = 'mechanic_diagnostics',
            icon = 'fa-solid fa-magnifying-glass',
            label = _U('target_diagnostics'),
            distance = 3.0,
            canInteract = function(entity)
                return IsPlayerOnDuty() and isVehicleDamaged(entity)
            end,
            onSelect = function(data)
                runDiagnostics(data.entity)
            end,
        },
    })
end)
