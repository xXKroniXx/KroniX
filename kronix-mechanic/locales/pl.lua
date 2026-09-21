Locales = {
    ['not_mechanic'] = 'Nie jesteś mechanikiem.',
    ['not_on_duty'] = 'Nie jesteś na służbie.',
    ['on_duty'] = 'Rozpocząłeś służbę jako mechanik.',
    ['off_duty'] = 'Zakończyłeś służbę.',
    ['boss_menu'] = 'Warsztat Mechaniczny',
    ['toggle_duty'] = 'Rozpocznij / zakończ służbę',
    ['open_shop'] = 'Sklep z częściami',
    ['open_garage'] = 'Garaż firmowy',
    ['open_jobboard'] = 'Tablica zleceń',

    ['target_diagnostics'] = 'Diagnozuj pojazd',
    ['target_repair'] = 'Napraw pojazd',
    ['target_tuning'] = 'Tuning',
    ['target_attach_tow'] = 'Podepnij do lawety',
    ['target_detach_tow'] = 'Odepnij pojazd',

    ['diagnosing'] = 'Diagnozowanie pojazdu...',
    ['repairing'] = 'Naprawianie pojazdu...',
    ['missing_diagnostic_tool'] = 'Potrzebujesz walizki diagnostycznej.',
    ['diagnostic_failed'] = 'Diagnostyka nie powiodła się, spróbuj ponownie.',
    ['diagnostic_success'] = 'Silnik: %d%%, Karoseria: %d%%. Wymagane zestawy naprawcze: %d',
    ['vehicle_not_damaged'] = 'Ten pojazd nie wymaga naprawy.',
    ['need_diagnostics_first'] = 'Najpierw zdiagnozuj pojazd.',
    ['missing_repair_kits'] = 'Potrzebujesz %d zestawów naprawczych.',
    ['repair_success'] = 'Pojazd został naprawiony.',
    ['repair_cancelled'] = 'Naprawa przerwana.',

    ['too_far_from_lift'] = 'Musisz być przy podnośniku, aby wykonać tuning.',
    ['not_enough_money'] = 'Nie masz wystarczająco pieniędzy.',
    ['tuning_menu'] = 'Menu tuningu',
    ['tuning_installed'] = 'Zainstalowano ulepszenie: %s (poziom %d)',
    ['tuning_maxed'] = 'To ulepszenie jest już na maksymalnym poziomie.',
    ['repaint_done'] = 'Pojazd został przemalowany.',

    ['no_tow_truck_nearby'] = 'Musisz znajdować się przy lawecie.',
    ['already_attached'] = 'Ten pojazd jest już podpięty.',
    ['nothing_attached'] = 'Do tej lawety nic nie jest podpięte.',
    ['vehicle_attached'] = 'Pojazd został podpięty do lawety.',
    ['vehicle_detached'] = 'Pojazd został odpięty.',

    ['garage_menu'] = 'Garaż firmowy',
    ['garage_take'] = 'Wyciągnij: %s',
    ['garage_store'] = 'Odstaw pojazd',
    ['already_have_vehicle'] = 'Masz już wyciągnięty pojazd firmowy.',
    ['not_job_vehicle'] = 'To nie jest pojazd firmowy.',
    ['too_far_from_garage'] = 'Musisz być przy garażu.',

    ['shop_menu'] = 'Sklep z częściami',
    ['already_own_tool'] = 'Posiadasz już walizkę diagnostyczną.',
    ['buy_amount'] = 'Podaj ilość',
    ['bought_item'] = 'Zakupiono: %s',

    ['jobboard_menu'] = 'Tablica zleceń',
    ['jobboard_empty'] = 'Brak dostępnych zleceń.',
    ['jobboard_entry'] = '%s - nagroda $%d',
    ['job_new'] = 'Nowe zlecenie dostępne na tablicy.',
    ['job_taken'] = 'To zlecenie zostało już przyjęte.',
    ['job_accepted'] = 'Przyjąłeś zlecenie. Uszkodzony pojazd oznaczony jest na mapie.',
    ['job_completed'] = 'Zlecenie ukończone! Otrzymujesz $%d.',
    ['job_active_already'] = 'Masz już aktywne zlecenie.',
    ['job_vehicle_blip'] = 'Uszkodzony pojazd',
}

function _U(key, ...)
    local str = Locales[key]
    if not str then return key end
    if select('#', ...) > 0 then
        return string.format(str, ...)
    end
    return str
end
