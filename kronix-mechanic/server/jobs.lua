local Jobs = {}
local jobIdCounter = 0
local OnDutyMechanics = {}

RegisterServerEvent('kronix-mechanic:server:setDuty')
AddEventHandler('kronix-mechanic:server:setDuty', function(state)
    local src = source
    local xPlayer = ESX.GetPlayerFromId(src)
    if not IsMechanic(xPlayer) then return end

    OnDutyMechanics[src] = state and true or nil
end)

AddEventHandler('playerDropped', function()
    local src = source
    OnDutyMechanics[src] = nil

    for id, job in pairs(Jobs) do
        if job.takenBy == src then
            Jobs[id] = nil
        end
    end
end)

local function countOnDuty()
    local count = 0
    for _ in pairs(OnDutyMechanics) do count = count + 1 end
    return count
end

local function jobsCount()
    local count = 0
    for _ in pairs(Jobs) do count = count + 1 end
    return count
end

local function createJob()
    jobIdCounter = jobIdCounter + 1
    local id = jobIdCounter

    local location = Config.JobBoard.locations[math.random(#Config.JobBoard.locations)]
    local model = Config.JobBoard.vehicles[math.random(#Config.JobBoard.vehicles)]
    local reward = math.random(Config.JobBoard.rewardMin, Config.JobBoard.rewardMax)

    Jobs[id] = {
        id = id,
        coords = location,
        model = model,
        reward = reward,
        taken = false,
        takenBy = nil,
    }

    for mechanicSource in pairs(OnDutyMechanics) do
        TriggerClientEvent('kronix-mechanic:client:newJob', mechanicSource)
    end
end

CreateThread(function()
    if not Config.JobBoard.enabled then return end

    while true do
        Wait(math.random(Config.JobBoard.intervalMin, Config.JobBoard.intervalMax))

        if countOnDuty() > 0 and jobsCount() < Config.JobBoard.maxActive then
            createJob()
        end
    end
end)

lib.callback.register('kronix-mechanic:server:getJobs', function(source)
    local xPlayer = ESX.GetPlayerFromId(source)
    if not IsMechanic(xPlayer) then return {} end

    local list = {}
    for _, job in pairs(Jobs) do
        if not job.taken then
            list[#list + 1] = job
        end
    end
    return list
end)

lib.callback.register('kronix-mechanic:server:acceptJob', function(source, jobId)
    local xPlayer = ESX.GetPlayerFromId(source)
    if not IsMechanic(xPlayer) then return false end

    local job = Jobs[jobId]
    if not job or job.taken then return false end

    job.taken = true
    job.takenBy = source

    return job
end)

lib.callback.register('kronix-mechanic:server:completeJob', function(source, jobId)
    local xPlayer = ESX.GetPlayerFromId(source)
    if not IsMechanic(xPlayer) then return false end

    local job = Jobs[jobId]
    if not job or job.takenBy ~= source then return false end

    xPlayer.addMoney(job.reward)
    Jobs[jobId] = nil

    return job.reward
end)
