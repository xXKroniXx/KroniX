ESX = exports['es_extended']:getSharedObject()

PlayerJob = nil
OnDuty = false

local bossPed = nil

local function isMechanic()
    return PlayerJob and PlayerJob.name == Config.Job
end

RegisterNetEvent('esx:playerLoaded', function(xPlayer)
    PlayerJob = xPlayer.job
end)

RegisterNetEvent('esx:setJob', function(job)
    PlayerJob = job
    if job.name ~= Config.Job and OnDuty then
        OnDuty = false
    end
end)

CreateThread(function()
    local xPlayer = ESX.GetPlayerData()
    PlayerJob = xPlayer.job
end)

local function spawnBoss()
    local model = GetHashKey('ig_bankman')
    lib.requestModel(model)
    bossPed = CreatePed(4, model, Config.MechanicShop.boss.x, Config.MechanicShop.boss.y, Config.MechanicShop.boss.z - 1.0, Config.MechanicShop.boss.w, false, true)
    SetEntityInvincible(bossPed, true)
    FreezeEntityPosition(bossPed, true)
    SetBlockingOfNonTemporaryEvents(bossPed, true)

    exports.ox_target:addLocalEntity(bossPed, {
        {
            name = 'mechanic_boss_menu',
            icon = 'fa-solid fa-screwdriver-wrench',
            label = _U('boss_menu'),
            distance = 2.5,
            onSelect = function()
                OpenBossMenu()
            end,
        },
    })
end

function OpenBossMenu()
    local options = {
        {
            title = _U('toggle_duty'),
            description = OnDuty and _U('off_duty') or _U('on_duty'),
            icon = 'clock',
            onSelect = function()
                ToggleDuty()
            end,
        },
        {
            title = _U('open_shop'),
            icon = 'cart-shopping',
            onSelect = function()
                OpenPartsShop()
            end,
        },
        {
            title = _U('open_garage'),
            icon = 'warehouse',
            onSelect = function()
                OpenGarageMenu()
            end,
        },
        {
            title = _U('open_jobboard'),
            icon = 'clipboard-list',
            onSelect = function()
                OpenJobBoard()
            end,
        },
    }

    lib.registerContext({
        id = 'mechanic_boss_menu',
        title = _U('boss_menu'),
        options = options,
    })
    lib.showContext('mechanic_boss_menu')
end

function ToggleDuty()
    if not isMechanic() then
        lib.notify({ description = _U('not_mechanic'), type = 'error' })
        return
    end

    OnDuty = not OnDuty
    TriggerServerEvent('kronix-mechanic:server:setDuty', OnDuty)
    lib.notify({ description = OnDuty and _U('on_duty') or _U('off_duty'), type = OnDuty and 'success' or 'inform' })
end

CreateThread(function()
    spawnBoss()

    local blip = AddBlipForCoord(Config.MechanicShop.boss.x, Config.MechanicShop.boss.y, Config.MechanicShop.boss.z)
    SetBlipSprite(blip, Config.MechanicShop.blip.sprite)
    SetBlipColour(blip, Config.MechanicShop.blip.color)
    SetBlipScale(blip, Config.MechanicShop.blip.scale)
    SetBlipAsShortRange(blip, true)
    BeginTextCommandSetBlipName('STRING')
    AddTextComponentString(Config.MechanicShop.blip.label)
    EndTextCommandSetBlipName(blip)
end)

function IsPlayerOnDuty()
    return OnDuty and isMechanic()
end

function IsNearAnyLift(coords)
    for _, lift in ipairs(Config.Lifts) do
        if #(coords - vector3(lift.x, lift.y, lift.z)) <= Config.LiftRadius then
            return true
        end
    end
    return false
end
