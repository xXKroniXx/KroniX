fx_version 'cerulean'
game 'gta5'

author 'KroniX'
description 'KroniX Mechanic - system pracy mechanika (ESX Legacy)'
version '1.0.0'

lua54 'yes'

shared_scripts {
    '@ox_lib/init.lua',
    'config.lua',
    'locales/pl.lua',
}

client_scripts {
    'client/main.lua',
    'client/diagnostics.lua',
    'client/repair.lua',
    'client/towing.lua',
    'client/tuning.lua',
    'client/jobs.lua',
    'client/garage.lua',
    'client/shop.lua',
}

server_scripts {
    '@oxmysql/lib/MySQL.lua',
    'server/main.lua',
    'server/jobs.lua',
    'server/shop.lua',
}

dependencies {
    'es_extended',
    'ox_lib',
    'ox_target',
    'oxmysql',
}
