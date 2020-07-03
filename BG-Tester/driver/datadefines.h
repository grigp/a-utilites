#ifndef DATA_DEFINES_H
#define DATA_DEFINES_H

namespace DataDefines {

/*!
 * \brief Значения частоты Frequency enum
 */
enum Frequency
{
      freq15Hz = 15
    , freq30Hz = 30
    , freq60Hz = 60
    , freq100Hz = 100
};

/*!
 * \brief Значения модуляции Modulation enum
 */
enum Modulation
{
      modNone = 0
    , modAm
    , modFm
    , modBoth
};

/*!
 * \brief Структура данных об энергии в каналах 1 - 12 PowerData struct
 */
struct PowerData
{
    int channel;
    int power;
    PowerData() {}
};


/*!
 * \brief Структура данных о включенных режимах ModeData struct
 */
struct ModeData
{
    Frequency freq;
    Modulation modulation;
    bool pause;
    bool reserve;
    ModeData() {}
};

/*!
 * \brief Структура данных об уникальном коде CodeUnique struct
 */
struct CodeUniqueData
{
    int code;
    CodeUniqueData() {}
};

/*!
 * \brief Структура данных об уровне заряда аккумулятора BatteryLevelData struct
 */
struct BatteryLevelData
{
    int level;
    BatteryLevelData() {}
};

}

#endif // DATA_DEFINES_H
