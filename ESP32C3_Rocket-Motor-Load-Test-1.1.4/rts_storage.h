/******************************************************************************
 *
 * Rocket Test Stand
 *
 * storage.h
 *
 * Gerenciamento das campanhas gravadas na Flash (LittleFS)
 *
 ******************************************************************************/

#ifndef RTS_STORAGE_H
#define RTS_STORAGE_H

#include <Arduino.h>
#include <LittleFS.h>

#include "campaign.h"
#include "config.h"

/******************************************************************************
 * Constantes
 ******************************************************************************/

#define STORAGE_HEADER_VERSION   2
#define STORAGE_INDEX_VERSION    2

#define STORAGE_INDEX_FILE       "/index.dat"

#define STORAGE_FILE_EXTENSION   ".rts"

/******************************************************************************
 * Cabeçalho gravado no início de cada arquivo de campanha
 ******************************************************************************/

struct StorageHeader
{
    uint32_t version;          // Versão da estrutura

    uint32_t campaignId;       // Número da campanha

    char dateTime[32];         // Data/hora recebida do navegador

    char description[51];      // Descrição do ensaio (50 caracteres + terminador)

    uint32_t sampleCount;      // Número de amostras gravadas

    float peakForce;           // Pico da campanha (gramas)
};

struct CampaignInfo
{
    uint32_t campaignId;
    char dateTime[32];
    char description[51];
    uint32_t sampleCount;
    float peakForce;
};

struct CampaignStatistics
{
    uint32_t campaignId;
    char dateTime[32];
    char description[51];
    uint32_t sampleCount;
    float peakForce;
    float minimumForce;
    float averageForce;
    float impulseGramSeconds;
    float durationSeconds;
};

/******************************************************************************
 * Classe Storage
 ******************************************************************************/

class Storage
{
public:

    Storage();

    //----------------------------------------------------------------------
    // Inicialização
    //----------------------------------------------------------------------

    bool begin();
    
    bool getCSVSize(uint32_t campaignId, size_t &csvSize) const;

    bool getCampaignInfo(uint32_t campaignId, CampaignInfo &info) const;

    bool getCampaignStatistics(uint32_t campaignId, CampaignStatistics &statistics) const;

    bool getCampaignJSONSize(uint32_t campaignId, size_t &jsonSize) const;

    bool streamCampaignJSON(uint32_t campaignId, Stream &stream) const;

    uint32_t getLastCampaignId() const;

    uint32_t getOldestCampaignId() const;

    bool deleteCampaign(uint32_t campaignId);

    bool updateCampaignDescription(uint32_t campaignId, const char *description);

    size_t getTotalBytes() const;

    size_t getUsedBytes() const;

    size_t getFreeBytes() const;

    bool exportCSV(uint32_t campaignId, Stream &stream);
    //----------------------------------------------------------------------
    // Obtém o próximo ID disponível
    //----------------------------------------------------------------------

    uint32_t nextCampaignId();

    //----------------------------------------------------------------------
    // Salva uma campanha completa
    //----------------------------------------------------------------------

    bool saveCampaign(
        uint32_t campaignId,
        const char *dateTime,
        const char *description,
        const Campaign &campaign);

    //----------------------------------------------------------------------
    // Carrega uma campanha da Flash
    //----------------------------------------------------------------------

    bool loadCampaign(
        uint32_t campaignId,
        Campaign &campaign);

    //----------------------------------------------------------------------
    // Verifica se uma campanha existe
    //----------------------------------------------------------------------

    bool exists(uint32_t campaignId) const;

    //----------------------------------------------------------------------
    // Quantidade de campanhas gravadas
    //----------------------------------------------------------------------

    uint32_t getCampaignCount() const;

    //----------------------------------------------------------------------
    // Lista todas as campanhas
    //----------------------------------------------------------------------

    void listCampaigns(Stream &out) const;

private:

    //----------------------------------------------------------------------
    // Cria index.dat
    //----------------------------------------------------------------------

    bool createIndexFile();

    //----------------------------------------------------------------------
    // Lê o ID atual
    //----------------------------------------------------------------------

    uint32_t readCurrentId() const;

    //----------------------------------------------------------------------
    // Atualiza index.dat
    //----------------------------------------------------------------------

    bool writeCurrentId(uint32_t id);

    bool ensureSpace(size_t requiredBytes);

    bool initializeStorageFormat();

    bool eraseCampaignFiles();
};

/******************************************************************************
 * Instância global
 ******************************************************************************/

extern Storage storage;

#endif