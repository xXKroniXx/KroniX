-- Wymagane tylko jeśli Config.Inventory = 'esx' (pomiń przy ox_inventory - tam przedmioty dodaje się w data/items.lua)
INSERT INTO `items` (`name`, `label`, `weight`, `type`, `image`, `unique`, `usable`, `price`, `rare`, `can_remove`)
VALUES
('diagnostictool', 'Walizka diagnostyczna', 1000, 'item', 'diagnostictool.png', 1, 0, 0, 0, 1),
('repairkit', 'Zestaw naprawczy', 500, 'item', 'repairkit.png', 0, 0, 0, 0, 1);
