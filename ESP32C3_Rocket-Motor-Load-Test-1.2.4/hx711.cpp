#include "hx711.h"

#include <Preferences.h>


HX711Driver Scale;


HX711Driver::HX711Driver() :
    loadCell(HX711_DOUT_PIN, HX711_SCK_PIN)
{

    grams = 0;

    raw = 0;

    inverted = false;

}



void HX711Driver::begin()
{

    Preferences preferences;
    if (preferences.begin("rocket-test", true))
    {
        inverted = preferences.getBool("force_inv", false);
        preferences.end();
    }

    Serial.println("Starting HX711");


    loadCell.begin();


    /*
       Tempo inicial para estabilização
    */
    loadCell.start(2000);


    /*
       Fator de calibração
       será ajustado depois pela interface web
    */
    loadCell.setCalFactor(HX711_SCALE);



    Serial.println("Tare...");

    loadCell.tare();



    Serial.print("Force sign...: ");
    Serial.println(inverted ? "INVERTED" : "NORMAL");

    Serial.println("HX711 ready");

}



void HX711Driver::tare()
{

    loadCell.tare();

}



bool HX711Driver::update()
{

    if(loadCell.update())
    {

        float measured = loadCell.getData();

        raw = (long)measured;
        grams = inverted ? -measured : measured;

        return true;

    }


    return false;

}



float HX711Driver::getGrams()
{

    return grams;

}



long HX711Driver::getRaw()
{

    return raw;

}

bool HX711Driver::isInverted() const
{
    return inverted;
}

bool HX711Driver::setInverted(bool value)
{
    if (inverted == value)
        return true;

    Preferences preferences;
    if (!preferences.begin("rocket-test", false))
        return false;

    size_t written = preferences.putBool("force_inv", value);
    preferences.end();

    if (written == 0)
        return false;

    inverted = value;

    // Evita que a interface mantenha um valor com o sinal antigo
    // até a próxima conversão do HX711.
    grams = -grams;

    Serial.print("Force sign changed to: ");
    Serial.println(inverted ? "INVERTED" : "NORMAL");

    return true;
}
