/******************************************************************************
 *
 * Rocket Test Stand
 *
 * storage.cpp
 *
 ******************************************************************************/

#include "rts_storage.h"

#include <Arduino.h>
#include <LittleFS.h>

#define INDEX_FILE "/index.dat"

Storage storage;

/******************************************************************************
 * Estrutura do arquivo de índice
 ******************************************************************************/

struct StorageIndex
{
    uint32_t nextId;
};

/******************************************************************************
 * Construtor
 ******************************************************************************/

Storage::Storage()
{
}

/******************************************************************************
 * Inicialização
 ******************************************************************************/

bool Storage::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("ERRO: LittleFS");
        return false;
    }

    if (!LittleFS.exists(STORAGE_INDEX_FILE))
    {
        if (!createIndexFile())
            return false;
    }

    Serial.println("LittleFS OK");

    return true;
}

/******************************************************************************
 * Exporta campanha em CSV
 ******************************************************************************/
bool Storage::exportCSV(uint32_t campaignId, Stream &stream)
{
    Campaign campaign;

    if (!loadCampaign(campaignId, campaign))
        return false;

    stream.println("time_us,force_g");

    const Sample *samples = campaign.data();

    for (uint32_t i = 0; i < campaign.getCount(); i++)
    {
        stream.print(samples[i].us);
        stream.print(',');
        stream.println(samples[i].grams, 2);
    }

    return true;
}
/******************************************************************************
 * Cria index.dat
 ******************************************************************************/

bool Storage::createIndexFile()
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_WRITE);

    if (!file)
        return false;

    StorageIndex index;

    index.nextId = 1;

    file.write(
        (uint8_t *)&index,
        sizeof(index));

    file.close();

    return true;
}

/******************************************************************************
 * Lê ID atual
 ******************************************************************************/

uint32_t Storage::readCurrentId() const
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_READ);

    if (!file)
        return 1;

    StorageIndex index;

    file.read(
        (uint8_t *)&index,
        sizeof(index));

    file.close();

    return index.nextId;
}

/******************************************************************************
 * Atualiza index.dat
 ******************************************************************************/

bool Storage::writeCurrentId(uint32_t id)
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_WRITE);

    if (!file)
        return false;

    StorageIndex index;

    index.nextId = id;

    file.write(
        (uint8_t *)&index,
        sizeof(index));

    file.close();

    return true;
}

/******************************************************************************
 * Próximo ID
 ******************************************************************************/

uint32_t Storage::nextCampaignId()
{
    uint32_t id = readCurrentId();

    writeCurrentId(id + 1);

    return id;
}

/******************************************************************************
 * Salva campanha
 ******************************************************************************/

bool Storage::saveCampaign(
        uint32_t campaignId,
        const char *dateTime,
        const Campaign &campaign)
{
    char filename[32];

    sprintf(
        filename,
        "/campaign%06lu.rts",
        (unsigned long)campaignId);

    File file = LittleFS.open(
        filename,
        FILE_WRITE);

    if (!file)
    {
        Serial.println("Erro criando arquivo.");

        return false;
    }

    StorageHeader header;

    memset(&header, 0, sizeof(header));

    header.version = STORAGE_HEADER_VERSION;

    header.campaignId = campaignId;

    strncpy(
        header.dateTime,
        dateTime,
        sizeof(header.dateTime) - 1);

    header.sampleCount = campaign.getCount();

    header.peakForce = campaign.getPeak();

    // Grava cabeçalho

    file.write(
        (uint8_t *)&header,
        sizeof(header));

    // Grava amostras

    file.write(
        (uint8_t *)campaign.data(),
        campaign.getCount() * sizeof(Sample));

    file.close();

    Serial.print("Campanha salva: ");

    Serial.println(filename);

    return true;
}

/******************************************************************************
 * Carrega campanha
 *
 * Será implementado quando iniciarmos o histórico.
 ******************************************************************************/
/******************************************************************************
 * Carrega campanha
 ******************************************************************************/

bool Storage::loadCampaign(
        uint32_t campaignId,
        Campaign &campaign)
{
    char filename[32];

    sprintf(
        filename,
        "/campaign%06lu.rts",
        (unsigned long)campaignId);

    File file = LittleFS.open(
        filename,
        FILE_READ);

    if (!file)
    {
        Serial.println("Erro abrindo campanha.");

        return false;
    }

    StorageHeader header;

    if (file.read(
            (uint8_t *)&header,
            sizeof(header)) != sizeof(header))
    {
        file.close();

        return false;
    }

    if (header.version != STORAGE_HEADER_VERSION)
    {
        file.close();

        return false;
    }

    campaign.clear();

    for (uint32_t i = 0; i < header.sampleCount; i++)
    {
        Sample sample;

        if (file.read(
                (uint8_t *)&sample,
                sizeof(sample)) != sizeof(sample))
        {
            file.close();

            return false;
        }

        campaign.start();

        campaign.addSample(
            sample.us,
            sample.grams);

        campaign.stop();
    }

    file.close();

    return true;
}

/******************************************************************************
 * Verifica existência
 ******************************************************************************/

bool Storage::exists(uint32_t campaignId) const
{
    char filename[32];

    sprintf(
        filename,
        "/campaign%06lu.rts",
        (unsigned long)campaignId);

    return LittleFS.exists(filename);
}

/******************************************************************************
 * Conta campanhas
 ******************************************************************************/

uint32_t Storage::getCampaignCount() const
{
    uint32_t count = 0;

    File root = LittleFS.open("/");

    File file = root.openNextFile();

    while (file)
    {
        String name = file.name();

        if (name.endsWith(".rts"))
            count++;

        file = root.openNextFile();
    }

    return count;
}

/******************************************************************************
 * Lista campanhas
 ******************************************************************************/

void Storage::listCampaigns(Stream &out) const
{
    File root = LittleFS.open("/");

    File file = root.openNextFile();

    while (file)
    {
        out.print(file.name());

        out.print("   ");

        out.print(file.size());

        out.println(" bytes");

        file = root.openNextFile();
    }
}