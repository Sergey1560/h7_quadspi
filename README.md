# STM32H7 QUADSPI
Пример работы с модулем QUADSPI.

Подключены два модуля флэш памяти W25Q16JV по 16Мбит в режиме Dual Flash Mode по 4-ем линия (Quad mode).
Управление CS одним сигналом от BK1.

В инициализации память очищается (chip erase), затем записывается массив из 300 байт с переходом через границу 
страницы.
После окончания записи флеш память подключается в режиме mapped.