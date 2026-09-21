PlayerVehicleNet = nil

local function hasActiveJobVehicle()
    return PlayerVehicleNet and DoesEntityExist(NetworkGetEntityFromNetworkId(PlayerVehicleNet))
end

local function spawnJobVehicle(model)
    if hasActiveJobVehicle() then
        lib.notify({ description = _U('already_have_vehicle'), type = 'error' })
        return
    end

    local hash = GetHashKey(model)
    lib.requestModel(hash)

    local spawn = Config.Garage.spawn
    local veh = CreateVehicle(hash, spawn.x, spawn.y, spawn.z, spawn.w, true, false)

    SetEntityAsMissionEntity(veh, true, true)
    SetVehicleHasBeenOwnedByPlayer(veh, true)
    SetVehicleNeedsToBeHotwired(veh, false)
    SetVehRadioStation(veh, 'OFF')
    SetModelAsNoLongerNeeded(hash)

    TaskWarpPedIntoVehicle(PlayerPedId(), veh, -1)
    PlayerVehicleNet = NetworkGetNetworkIdFromEntity(veh)
end

local function storeJobVehicle()
    local ped = PlayerPedId()
    if #(GetEntityCoords(ped) - vector3(Config.Garage.store.x, Config.Garage.store.y, Config.Garage.store.z)) > Config.Garage.radius then
        lib.notify({ description = _U('too_far_from_garage'), type = 'error' })
        return
    end

    if not hasActiveJobVehicle() then
        lib.notify({ description = _U('not_job_vehicle'), type = 'error' })
        return
    end

    local entity = NetworkGetEntityFromNetworkId(PlayerVehicleNet)
    if GetVehiclePedIsIn(ped, false) == entity then
        TaskLeaveVehicle(ped, entity, 0)
        Wait(800)
    end

    SetEntityAsMissionEntity(entity, true, true)
    DeleteEntity(entity)
    PlayerVehicleNet = nil
end

function OpenGarageMenu()
    if not IsPlayerOnDuty() then
        lib.notify({ description = _U('not_on_duty'), type = 'error' })
        return
    end

    local options = {}

    for _, jobVehicle in ipairs(Config.JobVehicles) do
        options[#options + 1] = {
            title = _U('garage_take', jobVehicle.label),
            icon = 'car',
            onSelect = function()
                spawnJobVehicle(jobVehicle.model)
            end,
        }
    end

    options[#options + 1] = {
        title = _U('garage_store'),
        icon = 'warehouse',
        onSelect = function()
            storeJobVehicle()
        end,
    }

    lib.registerContext({
        id = 'mechanic_garage_menu',
        title = _U('garage_menu'),
        options = options,
    })
    lib.showContext('mechanic_garage_menu')
end
