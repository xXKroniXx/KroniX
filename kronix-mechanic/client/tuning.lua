local function nextModLevel(vehicle, modType, maxLevels)
    local current = GetVehicleMod(vehicle, modType)
    local nextLevel = current + 1
    if nextLevel >= maxLevels then return nil end
    return nextLevel
end

function InstallTuning(vehicle, upgrade, level)
    local paid = lib.callback.await('kronix-mechanic:server:pay', false, upgrade.pricePerLevel)
    if not paid then
        lib.notify({ description = _U('not_enough_money'), type = 'error' })
        return
    end

    SetVehicleModKit(vehicle, 0)
    SetVehicleMod(vehicle, upgrade.modType, level, false)
    lib.notify({ description = _U('tuning_installed', upgrade.label, level + 1), type = 'success' })
end

function RepaintVehicle(vehicle)
    local input = lib.inputDialog(_U('tuning_menu'), {
        { type = 'color', label = 'Kolor', default = '#ffffff' },
    })

    if not input or not input[1] then return end

    local hex = input[1]
    local r, g, b
    if type(hex) == 'string' and hex:sub(1, 1) == '#' then
        r = tonumber(hex:sub(2, 3), 16)
        g = tonumber(hex:sub(4, 5), 16)
        b = tonumber(hex:sub(6, 7), 16)
    end

    if not (r and g and b) then return end

    local paid = lib.callback.await('kronix-mechanic:server:pay', false, Config.RepaintPrice)
    if not paid then
        lib.notify({ description = _U('not_enough_money'), type = 'error' })
        return
    end

    SetVehicleCustomPrimaryColour(vehicle, r, g, b)
    lib.notify({ description = _U('repaint_done'), type = 'success' })
end

local function openTuningMenu(vehicle)
    SetVehicleModKit(vehicle, 0)

    local options = {}

    for _, upgrade in ipairs(Config.Tuning) do
        local current = GetVehicleMod(vehicle, upgrade.modType)
        local currentLevel = math.max(0, current + 1)
        local nextLevel = nextModLevel(vehicle, upgrade.modType, upgrade.levels)

        options[#options + 1] = {
            title = ('%s (%d/%d)'):format(upgrade.label, currentLevel, upgrade.levels),
            description = nextLevel and ('$' .. upgrade.pricePerLevel) or _U('tuning_maxed'),
            icon = 'gears',
            disabled = nextLevel == nil,
            onSelect = function()
                if nextLevel then
                    InstallTuning(vehicle, upgrade, nextLevel)
                end
            end,
        }
    end

    options[#options + 1] = {
        title = 'Przemaluj pojazd',
        description = '$' .. Config.RepaintPrice,
        icon = 'paint-roller',
        onSelect = function()
            RepaintVehicle(vehicle)
        end,
    }

    lib.registerContext({
        id = 'mechanic_tuning_menu',
        title = _U('tuning_menu'),
        options = options,
    })
    lib.showContext('mechanic_tuning_menu')
end

CreateThread(function()
    exports.ox_target:addGlobalVehicle({
        {
            name = 'mechanic_tuning',
            icon = 'fa-solid fa-gears',
            label = _U('target_tuning'),
            distance = 3.0,
            canInteract = function(entity)
                if not IsPlayerOnDuty() then return false end
                return IsNearAnyLift(GetEntityCoords(entity))
            end,
            onSelect = function(data)
                openTuningMenu(data.entity)
            end,
        },
    })
end)
