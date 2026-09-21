Config = {}

Config.Job = 'mechanic' -- musi istnieć w tabeli `jobs` ESX (domyślnie jest)
Config.Inventory = 'ox_inventory' -- 'ox_inventory' albo 'esx'

Config.Items = {
    diagnosticTool = 'diagnostictool', -- narzędzie diagnostyczne (nie zużywa się)
    repairKit = 'repairkit', -- zestaw naprawczy (zużywalny)
}

Config.MechanicShop = {
    boss = vector4(-347.68, -133.66, 39.0, 70.0), -- ped szefa / punkt służby
    parts = vector3(-323.34, -139.85, 39.0), -- sklep z częściami
    blip = {
        sprite = 446,
        color = 69,
        scale = 0.8,
        label = 'Warsztat Mechaniczny',
    },
}

Config.Garage = {
    spawn = vector4(-345.0, -110.0, 38.7, 70.0), -- miejsce wyjazdu pojazdów firmowych
    store = vector3(-345.0, -110.0, 38.7), -- miejsce odstawienia pojazdów firmowych
    radius = 8.0,
}

Config.JobVehicles = {
    { model = 'flatbed', label = 'Laweta' },
    { model = 'speedo', label = 'Van serwisowy' },
}

Config.Lifts = { -- punkty podnośnika, wymagane do tuningu
    vector4(-320.0, -136.0, 39.0, 160.0),
    vector4(-314.0, -145.0, 39.0, 160.0),
}
Config.LiftRadius = 5.0

Config.Diagnostics = {
    duration = 8000,
    itemRequired = true,
    validForMs = 5 * 60 * 1000, -- diagnoza jest ważna 5 minut
}

Config.Repair = {
    duration = 12000,
    kitsPerEngineDamage = 100, -- 1 zestaw na każde 100 pkt brakującego zdrowia silnika
    kitsPerBodyDamage = 250, -- 1 zestaw na każde 250 pkt brakującego zdrowia karoserii
    minKits = 1,
}

Config.PartsShop = {
    { item = Config.Items.diagnosticTool, label = 'Walizka diagnostyczna', price = 250, oneTime = true },
    { item = Config.Items.repairKit, label = 'Zestaw naprawczy', price = 75, oneTime = false },
}

Config.Tuning = {
    { key = 'engine', label = 'Silnik', modType = 11, levels = 4, pricePerLevel = 2500 },
    { key = 'brakes', label = 'Hamulce', modType = 12, levels = 3, pricePerLevel = 1500 },
    { key = 'transmission', label = 'Skrzynia biegów', modType = 13, levels = 3, pricePerLevel = 2000 },
    { key = 'suspension', label = 'Zawieszenie', modType = 15, levels = 4, pricePerLevel = 1200 },
}
Config.RepaintPrice = 800

Config.Towing = {
    models = { 'flatbed' },
    attachOffset = vector3(0.0, -2.6, 1.05),
    attachRotation = vector3(0.0, 0.0, 0.0),
    maxAttachDistance = 6.0,
}

Config.JobBoard = {
    enabled = true,
    maxActive = 5,
    intervalMin = 3 * 60 * 1000,
    intervalMax = 8 * 60 * 1000,
    rewardMin = 300,
    rewardMax = 900,
    engineHealthOnSpawn = { min = 50.0, max = 250.0 },
    bodyHealthOnSpawn = { min = 200.0, max = 600.0 },
    vehicles = { 'blista', 'asea', 'premier', 'primo', 'washington' },
    locations = {
        vector4(-1130.0, -1520.0, 4.4, 30.0),
        vector4(215.0, -810.0, 30.6, 160.0),
        vector4(-550.0, -190.0, 38.0, 250.0),
        vector4(140.0, 620.0, 188.0, 210.0),
        vector4(-780.0, 280.0, 84.5, 0.0),
    },
}
