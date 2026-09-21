local function isVehicleDamaged(vehicle)
    local engine = GetVehicleEngineHealth(vehicle)
    local body = GetVehicleBodyHealth(vehicle)
    return engine < 990.0 or body < 990.0
end

local function runRepair(vehicle)
    if not IsPlayerOnDuty() then
        lib.notify({ description = _U('not_on_duty'), type = 'error' })
        return
    end

    if not isVehicleDamaged(vehicle) then
        lib.notify({ description = _U('vehicle_not_damaged'), type = 'inform' })
        return
    end

    local netId = NetworkGetNetworkIdFromEntity(vehicle)

    if not IsDiagnosisValid(netId) then
        lib.notify({ description = _U('need_diagnostics_first'), type = 'error' })
        return
    end

    local diagnosis = DiagnosedVehicles[netId]
    local kitsNeeded = CalculateRequiredKits(diagnosis.engine, diagnosis.body)

    local hasKits = lib.callback.await('kronix-mechanic:server:hasItem', false, Config.Items.repairKit, kitsNeeded)
    if not hasKits then
        lib.notify({ description = _U('missing_repair_kits', kitsNeeded), type = 'error' })
        return
    end

    local success = lib.progressBar({
        duration = Config.Repair.duration,
        label = _U('repairing'),
        useWhileDead = false,
        canCancel = true,
        disable = { move = true, car = true, combat = true },
        anim = { dict = 'mini@repair', clip = 'fixing_a_ped' },
    })

    if not success then
        lib.notify({ description = _U('repair_cancelled'), type = 'error' })
        return
    end

    local consumed = lib.callback.await('kronix-mechanic:server:consumeRepairKits', false, kitsNeeded)
    if not consumed then
        lib.notify({ description = _U('missing_repair_kits', kitsNeeded), type = 'error' })
        return
    end

    SetVehicleFixed(vehicle)
    SetVehicleEngineHealth(vehicle, 1000.0)
    SetVehicleBodyHealth(vehicle, 1000.0)
    SetVehicleDeformationFixed(vehicle)
    SetVehicleDirtLevel(vehicle, 0.0)

    DiagnosedVehicles[netId] = nil

    lib.notify({ description = _U('repair_success'), type = 'success' })

    if CheckJobVehicleRepaired then
        CheckJobVehicleRepaired(netId)
    end
end

CreateThread(function()
    exports.ox_target:addGlobalVehicle({
        {
            name = 'mechanic_repair',
            icon = 'fa-solid fa-wrench',
            label = _U('target_repair'),
            distance = 3.0,
            canInteract = function(entity)
                return IsPlayerOnDuty() and isVehicleDamaged(entity)
            end,
            onSelect = function(data)
                runRepair(data.entity)
            end,
        },
    })
end)
