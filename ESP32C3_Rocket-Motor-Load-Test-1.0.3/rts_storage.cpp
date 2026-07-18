#include "rts_storage.h"

#include <Arduino.h>
#include <LittleFS.h>

Storage storage;

struct StorageIndex
{
    uint32_t nextId;
};

Storage::Storage()
{
}

bool Storage::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("ERROR: LittleFS mount failed");
        return false;
    }

    if (!LittleFS.exists(STORAGE_INDEX_FILE) && !createIndexFile())
    {
        Serial.println("ERROR: index.dat creation failed");
        return false;
    }

    Serial.println("LittleFS OK");
    return true;
}

bool Storage::getCSVSize(uint32_t campaignId, size_t &csvSize) const
{
    csvSize = 0;

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    csvSize = strlen("time_us,force_g\r\n");

    char line[48];
    for (uint32_t i = 0; i < header.sampleCount; i++)
    {
        Sample sample;
        if (file.read((uint8_t *)&sample, sizeof(sample)) != sizeof(sample))
        {
            file.close();
            csvSize = 0;
            return false;
        }

        int length = snprintf(line, sizeof(line), "%lu,%.2f\r\n",
                              (unsigned long)sample.us,
                              (double)sample.grams);

        if (length <= 0 || length >= (int)sizeof(line))
        {
            file.close();
            csvSize = 0;
            return false;
        }

        csvSize += (size_t)length;

        if ((i & 0x7F) == 0)
            delay(0);
    }

    file.close();
    return true;
}

bool Storage::exportCSV(uint32_t campaignId, Stream &stream)
{
    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    static const char CSV_HEADER[] = "time_us,force_g\r\n";
    if (stream.write((const uint8_t *)CSV_HEADER, sizeof(CSV_HEADER) - 1) !=
        sizeof(CSV_HEADER) - 1)
    {
        file.close();
        return false;
    }

    char outputBuffer[512];
    size_t outputLength = 0;
    char line[48];

    for (uint32_t i = 0; i < header.sampleCount; i++)
    {
        Sample sample;
        if (file.read((uint8_t *)&sample, sizeof(sample)) != sizeof(sample))
        {
            file.close();
            return false;
        }

        int lineLength = snprintf(line, sizeof(line), "%lu,%.2f\r\n",
                                  (unsigned long)sample.us,
                                  (double)sample.grams);

        if (lineLength <= 0 || lineLength >= (int)sizeof(line))
        {
            file.close();
            return false;
        }

        if (outputLength + (size_t)lineLength > sizeof(outputBuffer))
        {
            if (stream.write((const uint8_t *)outputBuffer, outputLength) != outputLength)
            {
                file.close();
                return false;
            }

            outputLength = 0;
            delay(0);
        }

        memcpy(outputBuffer + outputLength, line, (size_t)lineLength);
        outputLength += (size_t)lineLength;
    }

    if (outputLength > 0 &&
        stream.write((const uint8_t *)outputBuffer, outputLength) != outputLength)
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool Storage::createIndexFile()
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_WRITE);
    if (!file)
        return false;

    StorageIndex index = {1};
    size_t written = file.write((const uint8_t *)&index, sizeof(index));
    file.close();

    return written == sizeof(index);
}

uint32_t Storage::readCurrentId() const
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_READ);
    if (!file)
        return 1;

    StorageIndex index = {1};
    size_t read = file.read((uint8_t *)&index, sizeof(index));
    file.close();

    if (read != sizeof(index) || index.nextId == 0)
        return 1;

    return index.nextId;
}

bool Storage::writeCurrentId(uint32_t id)
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_WRITE);
    if (!file)
        return false;

    StorageIndex index = {id};
    size_t written = file.write((const uint8_t *)&index, sizeof(index));
    file.close();

    return written == sizeof(index);
}

uint32_t Storage::nextCampaignId()
{
    uint32_t id = readCurrentId();

    if (!writeCurrentId(id + 1))
        Serial.println("WARNING: unable to update campaign index");

    return id;
}

bool Storage::saveCampaign(uint32_t campaignId,
                           const char *dateTime,
                           const Campaign &source)
{
    if (source.getCount() == 0 || source.getCount() > MAX_SAMPLES)
        return false;

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.println("Error creating campaign file");
        return false;
    }

    StorageHeader header;
    memset(&header, 0, sizeof(header));
    header.version = STORAGE_HEADER_VERSION;
    header.campaignId = campaignId;
    header.sampleCount = source.getCount();
    header.peakForce = source.getPeak();

    if (dateTime != nullptr)
        strncpy(header.dateTime, dateTime, sizeof(header.dateTime) - 1);

    size_t headerWritten = file.write((const uint8_t *)&header, sizeof(header));
    size_t dataSize = source.getCount() * sizeof(Sample);
    size_t dataWritten = file.write((const uint8_t *)source.data(), dataSize);
    file.close();

    if (headerWritten != sizeof(header) || dataWritten != dataSize)
    {
        LittleFS.remove(filename);
        Serial.println("Error writing campaign file");
        return false;
    }

    Serial.print("Campaign saved: ");
    Serial.println(filename);
    return true;
}

bool Storage::loadCampaign(uint32_t campaignId, Campaign &destination)
{
    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    destination.start();

    for (uint32_t i = 0; i < header.sampleCount; i++)
    {
        Sample sample;
        if (file.read((uint8_t *)&sample, sizeof(sample)) != sizeof(sample) ||
            !destination.addSample(sample.us, sample.grams))
        {
            destination.clear();
            file.close();
            return false;
        }
    }

    destination.stop();
    file.close();
    return true;
}

bool Storage::exists(uint32_t campaignId) const
{
    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);
    return LittleFS.exists(filename);
}

uint32_t Storage::getCampaignCount() const
{
    uint32_t count = 0;
    File root = LittleFS.open("/");
    if (!root)
        return 0;

    File file = root.openNextFile();
    while (file)
    {
        String name = file.name();
        if (!file.isDirectory() && name.endsWith(STORAGE_FILE_EXTENSION))
            count++;
        file.close();
        file = root.openNextFile();
    }
    root.close();

    return count;
}

void Storage::listCampaigns(Stream &out) const
{
    File root = LittleFS.open("/");
    if (!root)
        return;

    File file = root.openNextFile();
    while (file)
    {
        String name = file.name();
        if (!file.isDirectory() && name.endsWith(STORAGE_FILE_EXTENSION))
        {
            out.print(name);
            out.print("   ");
            out.print(file.size());
            out.println(" bytes");
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}
