ActiveJob = nil

local function randomBetween(min, max)
    return min + (max - min) * math.random()
end

local function spawnJobVehicle(job)
    local hash = GetHashKey(job.model)
    lib.requestModel(hash)

    local coords = job.coords
    local veh = CreateVehicle(hash, coords.x, coords.y, coords.z, coords.w, true, false)

    SetEntityAsMissionEntity(veh, true, true)
    SetModelAsNoLongerNeeded(hash)

    local engineHealth = randomBetween(Config.JobBoard.engineHealthOnSpawn.min, Config.JobBoard.engineHealthOnSpawn.max)
    local bodyHealth = randomBetween(Config.JobBoard.bodyHealthOnSpawn.min, Config.JobBoard.bodyHealthOnSpawn.max)

    SetVehicleEngineHealth(veh, engineHealth)
    SetVehicleBodyHealth(veh, bodyHealth)
    SetVehicleDirtLevel(veh, 10.0)

    job.netId = NetworkGetNetworkIdFromEntity(veh)

    job.blip = AddBlipForCoord(coords.x, coords.y, coords.z)
    SetBlipSprite(job.blip, 446)
    SetBlipColour(job.blip, 1)
    SetBlipScale(job.blip, 0.9)
    SetBlipAsShortRange(job.blip, false)
    BeginTextCommandSetBlipName('STRING')
    AddTextComponentString(_U('job_vehicle_blip'))
    EndTextCommandSetBlipName(job.blip)
end

local function cleanupJob()
    if ActiveJob then
        if ActiveJob.blip then
            RemoveBlip(ActiveJob.blip)
        end
        if ActiveJob.netId then
            local entity = NetworkGetEntityFromNetworkId(ActiveJob.netId)
            if DoesEntityExist(entity) then
                SetTimeout(120000, function()
                    if DoesEntityExist(entity) and IsEntityAVehicle(entity) and GetPedInVehicleSeat(entity, -1) == 0 then
                        SetEntityAsMissionEntity(entity, true, true)
                        DeleteEntity(entity)
                    end
                end)
            end
        end
    end
    ActiveJob = nil
end

function AcceptJob(jobId)
    local job = lib.callback.await('kronix-mechanic:server:acceptJob', false, jobId)
    if not job then
        lib.notify({ description = _U('job_taken'), type = 'error' })
        return
    end

    ActiveJob = job
    spawnJobVehicle(ActiveJob)
    lib.notify({ description = _U('job_accepted'), type = 'success' })
end

function CheckJobVehicleRepaired(netId)
    if ActiveJob and ActiveJob.netId == netId then
        local reward = lib.callback.await('kronix-mechanic:server:completeJob', false, ActiveJob.id)
        if reward then
            lib.notify({ description = _U('job_completed', reward), type = 'success' })
        end
        cleanupJob()
    end
end

function OpenJobBoard()
    if not IsPlayerOnDuty() then
        lib.notify({ description = _U('not_on_duty'), type = 'error' })
        return
    end

    if ActiveJob then
        lib.notify({ description = _U('job_active_already'), type = 'error' })
        return
    end

    local jobs = lib.callback.await('kronix-mechanic:server:getJobs', false)

    if not jobs or #jobs == 0 then
        lib.notify({ description = _U('jobboard_empty'), type = 'inform' })
        return
    end

    local options = {}
    for _, job in ipairs(jobs) do
        options[#options + 1] = {
            title = _U('jobboard_entry', job.model, job.reward),
            icon = 'car-burst',
            onSelect = function()
                AcceptJob(job.id)
            end,
        }
    end

    lib.registerContext({
        id = 'mechanic_jobboard_menu',
        title = _U('jobboard_menu'),
        options = options,
    })
    lib.showContext('mechanic_jobboard_menu')
end

RegisterNetEvent('kronix-mechanic:client:newJob', function()
    lib.notify({ description = _U('job_new'), type = 'inform' })
end)

AddEventHandler('onResourceStop', function(resourceName)
    if GetCurrentResourceName() == resourceName then
        cleanupJob()
    end
end)
