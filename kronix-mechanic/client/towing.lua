AttachedVehicles = {} -- [towNetId] = targetNetId

local function isTowTruckModel(vehicle)
    local model = GetEntityModel(vehicle)
    for _, m in ipairs(Config.Towing.models) do
        if model == GetHashKey(m) then return true end
    end
    return false
end

local function getNearbyVehicles(coords, maxDistance)
    local vehicles = {}
    for _, vehicle in ipairs(GetGamePool('CVehicle')) do
        if DoesEntityExist(vehicle) and #(coords - GetEntityCoords(vehicle)) <= maxDistance then
            vehicles[#vehicles + 1] = vehicle
        end
    end
    return vehicles
end

local function findNearbyTowTruck(ped)
    local vehicle = GetVehiclePedIsIn(ped, false)
    if vehicle ~= 0 and isTowTruckModel(vehicle) then
        return vehicle
    end

    local coords = GetEntityCoords(ped)
    for _, veh in ipairs(getNearbyVehicles(coords, Config.Towing.maxAttachDistance)) do
        if isTowTruckModel(veh) then
            return veh
        end
    end

    return nil
end

local function isAlreadyAttached(targetNetId)
    for _, attachedTarget in pairs(AttachedVehicles) do
        if attachedTarget == targetNetId then return true end
    end
    return false
end

local function attachToTow(target)
    if not IsPlayerOnDuty() then
        lib.notify({ description = _U('not_on_duty'), type = 'error' })
        return
    end

    local ped = PlayerPedId()
    local towTruck = findNearbyTowTruck(ped)
    if not towTruck then
        lib.notify({ description = _U('no_tow_truck_nearby'), type = 'error' })
        return
    end

    local towNetId = NetworkGetNetworkIdFromEntity(towTruck)
    local targetNetId = NetworkGetNetworkIdFromEntity(target)

    if isAlreadyAttached(targetNetId) then
        lib.notify({ description = _U('already_attached'), type = 'error' })
        return
    end

    local offset = Config.Towing.attachOffset
    local rot = Config.Towing.attachRotation

    AttachEntityToEntity(target, towTruck, 0, offset.x, offset.y, offset.z, rot.x, rot.y, rot.z, true, true, false, false, 2, true)

    AttachedVehicles[towNetId] = targetNetId
    lib.notify({ description = _U('vehicle_attached'), type = 'success' })
end

local function detachFromTow(towTruck)
    local towNetId = NetworkGetNetworkIdFromEntity(towTruck)
    local targetNetId = AttachedVehicles[towNetId]

    if not targetNetId then
        lib.notify({ description = _U('nothing_attached'), type = 'error' })
        return
    end

    local target = NetworkGetEntityFromNetworkId(targetNetId)
    if DoesEntityExist(target) then
        DetachEntity(target, true, false)
    end

    AttachedVehicles[towNetId] = nil
    lib.notify({ description = _U('vehicle_detached'), type = 'success' })
end

CreateThread(function()
    exports.ox_target:addGlobalVehicle({
        {
            name = 'mechanic_tow_attach',
            icon = 'fa-solid fa-link',
            label = _U('target_attach_tow'),
            distance = 3.0,
            canInteract = function(entity)
                if not IsPlayerOnDuty() or isTowTruckModel(entity) then return false end
                return not isAlreadyAttached(NetworkGetNetworkIdFromEntity(entity))
            end,
            onSelect = function(data)
                attachToTow(data.entity)
            end,
        },
        {
            name = 'mechanic_tow_detach',
            icon = 'fa-solid fa-link-slash',
            label = _U('target_detach_tow'),
            distance = 3.0,
            canInteract = function(entity)
                if not isTowTruckModel(entity) then return false end
                return AttachedVehicles[NetworkGetNetworkIdFromEntity(entity)] ~= nil
            end,
            onSelect = function(data)
                detachFromTow(data.entity)
            end,
        },
    })
end)
