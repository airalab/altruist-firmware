#ifdef ALTRUIST_INSIGHT

#ifndef DISPLAY_MODES_H
#define DISPLAY_MODES_H

// Высокоуровневые режимы работы дисплея.
// Они мапятся на конкретные функции драйвера (Init, Init_Fast, Init_4Gray, PartialDisplay и т.п.).
enum class DisplayMode {
    FULL,       // Полное обновление (Init + Display)
    FAST,       // Быстрое обновление (Init_Fast + Display_Fast)
    PARTIAL,    // Частичное обновление (Init_Fast + PartialDisplay)
    GRAY_4      // 4-х уровневый серый (Init_4Gray + Display_4Gray)
};

#endif // DISPLAY_MODES_H

#endif


