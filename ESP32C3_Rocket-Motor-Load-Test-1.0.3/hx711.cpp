#include "hx711.h"


HX711Driver Scale;


HX711Driver::HX711Driver() :
    loadCell(HX711_DOUT_PIN, HX711_SCK_PIN)
{

    grams = 0;

    raw = 0;

}



void HX711Driver::begin()
{

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

        grams = loadCell.getData();

        raw = loadCell.getData();

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