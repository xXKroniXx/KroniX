local function buyOneTime(entry)
    local hasIt = lib.callback.await('kronix-mechanic:server:hasItem', false, entry.item, 1)
    if hasIt then
        lib.notify({ description = _U('already_own_tool'), type = 'error' })
        return
    end

    local bought = lib.callback.await('kronix-mechanic:server:buyItem', false, entry.item, 1, entry.price)
    if bought then
        lib.notify({ description = _U('bought_item', entry.label), type = 'success' })
    else
        lib.notify({ description = _U('not_enough_money'), type = 'error' })
    end
end

local function buyStackable(entry)
    local input = lib.inputDialog(entry.label, {
        { type = 'number', label = _U('buy_amount'), default = 1, min = 1, max = 50 },
    })

    if not input or not input[1] then return end

    local amount = math.floor(tonumber(input[1]) or 0)
    if amount < 1 then return end

    local bought = lib.callback.await('kronix-mechanic:server:buyItem', false, entry.item, amount, entry.price * amount)
    if bought then
        lib.notify({ description = _U('bought_item', ('%s x%d'):format(entry.label, amount)), type = 'success' })
    else
        lib.notify({ description = _U('not_enough_money'), type = 'error' })
    end
end

function OpenPartsShop()
    if not IsPlayerOnDuty() then
        lib.notify({ description = _U('not_on_duty'), type = 'error' })
        return
    end

    local options = {}

    for _, entry in ipairs(Config.PartsShop) do
        options[#options + 1] = {
            title = entry.label,
            description = entry.oneTime and ('$' .. entry.price) or ('$' .. entry.price .. ' / szt.'),
            icon = entry.oneTime and 'toolbox' or 'box',
            onSelect = function()
                if entry.oneTime then
                    buyOneTime(entry)
                else
                    buyStackable(entry)
                end
            end,
        }
    end

    lib.registerContext({
        id = 'mechanic_parts_shop',
        title = _U('shop_menu'),
        options = options,
    })
    lib.showContext('mechanic_parts_shop')
end
