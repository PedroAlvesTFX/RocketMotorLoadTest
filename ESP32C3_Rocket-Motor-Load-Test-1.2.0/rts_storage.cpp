#include "rts_storage.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <float.h>

Storage storage;

struct StorageIndex
{
    uint32_t version;
    uint32_t nextId;
};

Storage::Storage() : importExpectedBytes(0), importReceivedBytes(0)
{
}

bool Storage::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("ERROR: LittleFS mount failed");
        return false;
    }

    if (!initializeStorageFormat())
    {
        Serial.println("ERROR: storage format initialization failed");
        return false;
    }

    Serial.println("LittleFS OK");
    return true;
}

bool Storage::getCampaignInfo(uint32_t campaignId, CampaignInfo &info) const
{
    memset(&info, 0, sizeof(info));

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    size_t bytesRead = file.read((uint8_t *)&header, sizeof(header));
    file.close();

    if (bytesRead != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.campaignId != campaignId ||
        header.sampleCount > MAX_SAMPLES)
    {
        return false;
    }

    info.campaignId = header.campaignId;
    info.sampleCount = header.sampleCount;
    info.peakForce = header.peakForce;
    strncpy(info.dateTime, header.dateTime, sizeof(info.dateTime) - 1);
    info.dateTime[sizeof(info.dateTime) - 1] = '\0';
    strncpy(info.description, header.description, sizeof(info.description) - 1);
    info.description[sizeof(info.description) - 1] = '\0';

    return true;
}


bool Storage::getCampaignStatistics(uint32_t campaignId,
                                    CampaignStatistics &statistics) const
{
    memset(&statistics, 0, sizeof(statistics));

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.campaignId != campaignId ||
        header.sampleCount == 0 ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    Sample previous;
    if (file.read((uint8_t *)&previous, sizeof(previous)) != sizeof(previous))
    {
        file.close();
        return false;
    }

    float minimum = previous.grams;
    float maximum = previous.grams;
    double sum = previous.grams;
    double positiveImpulse = 0.0;
    uint32_t firstUs = previous.us;
    uint32_t lastUs = previous.us;
    uint32_t peakUs = previous.us;
    uint32_t ignitionUs = 0;
    uint32_t burnEndUs = 0;
    bool ignitionFound = previous.grams >= TRIGGER_GRAMS;
    if (ignitionFound)
    {
        ignitionUs = previous.us;
        burnEndUs = previous.us;
    }

    for (uint32_t i = 1; i < header.sampleCount; ++i)
    {
        Sample current;
        if (file.read((uint8_t *)&current, sizeof(current)) != sizeof(current))
        {
            file.close();
            return false;
        }

        if (current.grams < minimum)
            minimum = current.grams;
        if (current.grams > maximum)
        {
            maximum = current.grams;
            peakUs = current.us;
        }

        sum += current.grams;

        uint32_t deltaUs = current.us - previous.us;
        float previousPositive = previous.grams > 0.0f ? previous.grams : 0.0f;
        float currentPositive = current.grams > 0.0f ? current.grams : 0.0f;
        positiveImpulse += ((double)previousPositive + (double)currentPositive) * 0.5 *
                           ((double)deltaUs / 1000000.0);

        if (!ignitionFound && current.grams >= TRIGGER_GRAMS)
        {
            ignitionFound = true;
            ignitionUs = current.us;
            burnEndUs = current.us;
        }

        if (ignitionFound && current.grams > STOP_GRAMS)
            burnEndUs = current.us;

        previous = current;
        lastUs = current.us;

        if ((i & 0x7F) == 0)
            delay(0);
    }

    file.close();

    const float duration = (float)(lastUs - firstUs) / 1000000.0f;
    const float ignitionTime = ignitionFound ? (float)(ignitionUs - firstUs) / 1000000.0f : 0.0f;
    const float peakTime = (float)(peakUs - firstUs) / 1000000.0f;
    const float timeToPeak = ignitionFound && peakUs >= ignitionUs
                                 ? (float)(peakUs - ignitionUs) / 1000000.0f
                                 : 0.0f;
    const float burnTime = ignitionFound && burnEndUs >= ignitionUs
                               ? (float)(burnEndUs - ignitionUs) / 1000000.0f
                               : 0.0f;

    statistics.campaignId = header.campaignId;
    statistics.sampleCount = header.sampleCount;
    statistics.peakForce = maximum;
    statistics.minimumForce = minimum;
    statistics.averageForce = (float)(sum / (double)header.sampleCount);
    statistics.impulseGramSeconds = (float)positiveImpulse;
    statistics.durationSeconds = duration;
    statistics.ignitionTimeSeconds = ignitionTime;
    statistics.peakTimeSeconds = peakTime;
    statistics.timeToPeakSeconds = timeToPeak;
    statistics.burnTimeSeconds = burnTime;
    statistics.burnAverageForce = burnTime > 0.0f
                                      ? (float)(positiveImpulse / (double)burnTime)
                                      : 0.0f;
    statistics.sampleRateSps = duration > 0.0f && header.sampleCount > 1
                                   ? (float)(header.sampleCount - 1) / duration
                                   : 0.0f;
    strncpy(statistics.dateTime, header.dateTime, sizeof(statistics.dateTime) - 1);
    statistics.dateTime[sizeof(statistics.dateTime) - 1] = '\0';
    strncpy(statistics.description, header.description, sizeof(statistics.description) - 1);
    statistics.description[sizeof(statistics.description) - 1] = '\0';

    return true;
}

bool Storage::getCampaignJSONSize(uint32_t campaignId, size_t &jsonSize) const
{
    jsonSize = strlen("{\"samples\":[") + strlen("]}");

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.campaignId != campaignId ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    char buffer[96];
    for (uint32_t i = 0; i < header.sampleCount; ++i)
    {
        Sample sample;
        if (file.read((uint8_t *)&sample, sizeof(sample)) != sizeof(sample))
        {
            file.close();
            jsonSize = 0;
            return false;
        }

        int length = snprintf(buffer, sizeof(buffer), "%s[%lu,%.2f]",
                              i == 0 ? "" : ",",
                              (unsigned long)sample.us,
                              (double)sample.grams);
        if (length <= 0 || length >= (int)sizeof(buffer))
        {
            file.close();
            jsonSize = 0;
            return false;
        }
        jsonSize += (size_t)length;
        if ((i & 0x7F) == 0)
            delay(0);
    }

    file.close();
    return true;
}

bool Storage::streamCampaignJSON(uint32_t campaignId, Stream &stream) const
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
        header.campaignId != campaignId ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    if (stream.print("{\"samples\":[") == 0)
    {
        file.close();
        return false;
    }

    char buffer[96];
    for (uint32_t i = 0; i < header.sampleCount; ++i)
    {
        Sample sample;
        if (file.read((uint8_t *)&sample, sizeof(sample)) != sizeof(sample))
        {
            file.close();
            return false;
        }

        int length = snprintf(buffer, sizeof(buffer),
                              "%s[%lu,%.2f]",
                              i == 0 ? "" : ",",
                              (unsigned long)sample.us,
                              (double)sample.grams);

        if (length <= 0 || length >= (int)sizeof(buffer) ||
            stream.write((const uint8_t *)buffer, (size_t)length) != (size_t)length)
        {
            file.close();
            return false;
        }

        if ((i & 0x3F) == 0)
            delay(0);
    }

    file.close();
    return stream.print("]}") > 0;
}

uint32_t Storage::getLastCampaignId() const
{
    uint32_t nextId = readCurrentId();
    if (nextId <= 1)
        return 0;

    for (uint32_t id = nextId - 1; id > 0; --id)
    {
        if (exists(id))
            return id;
    }

    return 0;
}

uint32_t Storage::getOldestCampaignId() const
{
    uint32_t nextId = readCurrentId();
    for (uint32_t id = 1; id < nextId; ++id)
    {
        if (exists(id))
            return id;
    }

    return 0;
}

bool Storage::deleteCampaign(uint32_t campaignId)
{
    if (campaignId == 0)
        return false;

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    if (!LittleFS.exists(filename))
        return false;

    bool removed = LittleFS.remove(filename);
    if (removed)
    {
        Serial.print("Campaign deleted: ");
        Serial.println(filename);
    }

    return removed;
}

bool Storage::updateCampaignDescription(uint32_t campaignId,
                                        const char *description)
{
    if (campaignId == 0)
        return false;

    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, "r+");
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.campaignId != campaignId ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    memset(header.description, 0, sizeof(header.description));
    if (description != nullptr)
        strncpy(header.description, description, sizeof(header.description) - 1);

    if (!file.seek(0, SeekSet))
    {
        file.close();
        return false;
    }

    const size_t written = file.write((const uint8_t *)&header, sizeof(header));
    file.flush();
    file.close();

    if (written != sizeof(header))
        return false;

    Serial.print("Campaign description updated: ");
    Serial.println(filename);
    return true;
}

size_t Storage::getTotalBytes() const
{
    return LittleFS.totalBytes();
}

size_t Storage::getUsedBytes() const
{
    return LittleFS.usedBytes();
}

size_t Storage::getFreeBytes() const
{
    size_t total = getTotalBytes();
    size_t used = getUsedBytes();
    return used < total ? total - used : 0;
}

bool Storage::ensureSpace(size_t requiredBytes)
{
    const size_t targetFree = requiredBytes + (size_t)STORAGE_MIN_FREE_BYTES;

    if (targetFree > getTotalBytes())
    {
        Serial.println("ERROR: campaign is larger than available filesystem");
        return false;
    }

    while (getFreeBytes() < targetFree)
    {
        uint32_t oldestId = getOldestCampaignId();
        if (oldestId == 0)
        {
            Serial.println("ERROR: insufficient storage and no campaign can be deleted");
            return false;
        }

        Serial.print("Storage low. Deleting oldest campaign: ");
        Serial.println(oldestId);

        if (!deleteCampaign(oldestId))
        {
            Serial.println("ERROR: unable to delete oldest campaign");
            return false;
        }

        delay(0);
    }

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

    char metadata[192];
    int metadataLength = snprintf(
        metadata,
        sizeof(metadata),
        "# campaign,%lu\r\n# datetime,%s\r\n# description,%s\r\ntime_us,force_g\r\n",
        (unsigned long)header.campaignId,
        header.dateTime,
        header.description);

    if (metadataLength <= 0 || metadataLength >= (int)sizeof(metadata))
    {
        file.close();
        return false;
    }

    csvSize = (size_t)metadataLength;

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

    char metadata[192];
    int metadataLength = snprintf(
        metadata,
        sizeof(metadata),
        "# campaign,%lu\r\n# datetime,%s\r\n# description,%s\r\ntime_us,force_g\r\n",
        (unsigned long)header.campaignId,
        header.dateTime,
        header.description);

    if (metadataLength <= 0 || metadataLength >= (int)sizeof(metadata) ||
        stream.write((const uint8_t *)metadata, (size_t)metadataLength) != (size_t)metadataLength)
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

bool Storage::eraseCampaignFiles()
{
    File root = LittleFS.open("/");
    if (!root)
        return false;

    File file = root.openNextFile();
    while (file)
    {
        String name = file.name();
        file.close();

        if (name.endsWith(STORAGE_FILE_EXTENSION))
        {
            if (!name.startsWith("/"))
                name = "/" + name;
            LittleFS.remove(name);
        }

        file = root.openNextFile();
        delay(0);
    }

    root.close();
    return true;
}

bool Storage::initializeStorageFormat()
{
    bool validIndex = false;

    if (LittleFS.exists(STORAGE_INDEX_FILE))
    {
        File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_READ);
        if (file)
        {
            StorageIndex index = {0, 0};
            size_t bytesRead = file.read((uint8_t *)&index, sizeof(index));
            file.close();

            validIndex = bytesRead == sizeof(index) &&
                         index.version == STORAGE_INDEX_VERSION &&
                         index.nextId > 0;
        }
    }

    if (validIndex)
        return true;

    Serial.println("Storage format changed. Erasing previous campaigns...");
    eraseCampaignFiles();
    LittleFS.remove(STORAGE_INDEX_FILE);
    return createIndexFile();
}

bool Storage::createIndexFile()
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_WRITE);
    if (!file)
        return false;

    StorageIndex index = {STORAGE_INDEX_VERSION, 1};
    size_t written = file.write((const uint8_t *)&index, sizeof(index));
    file.close();

    return written == sizeof(index);
}

uint32_t Storage::readCurrentId() const
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_READ);
    if (!file)
        return 1;

    StorageIndex index = {0, 1};
    size_t read = file.read((uint8_t *)&index, sizeof(index));
    file.close();

    if (read != sizeof(index) ||
        index.version != STORAGE_INDEX_VERSION ||
        index.nextId == 0)
    {
        return 1;
    }

    return index.nextId;
}

bool Storage::writeCurrentId(uint32_t id)
{
    File file = LittleFS.open(STORAGE_INDEX_FILE, FILE_WRITE);
    if (!file)
        return false;

    StorageIndex index = {STORAGE_INDEX_VERSION, id};
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
                           const char *description,
                           const Campaign &source)
{
    if (source.getCount() == 0 || source.getCount() > MAX_SAMPLES)
        return false;

    const size_t requiredBytes = sizeof(StorageHeader) +
                                 source.getCount() * sizeof(Sample);

    if (!ensureSpace(requiredBytes))
    {
        Serial.println("ERROR: insufficient space to save campaign");
        return false;
    }

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
    header.dateTime[sizeof(header.dateTime) - 1] = '\0';

    if (description != nullptr)
        strncpy(header.description, description, sizeof(header.description) - 1);
    header.description[sizeof(header.description) - 1] = '\0';

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

bool Storage::getRTSSize(uint32_t campaignId, size_t &rtsSize) const
{
    rtsSize = 0;
    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.campaignId != campaignId ||
        header.sampleCount == 0 ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        return false;
    }

    const size_t expected = sizeof(StorageHeader) +
                            (size_t)header.sampleCount * sizeof(Sample);
    const size_t actual = file.size();
    file.close();

    if (actual != expected)
        return false;

    rtsSize = actual;
    return true;
}

bool Storage::exportRTS(uint32_t campaignId, Stream &stream) const
{
    char filename[32];
    snprintf(filename, sizeof(filename), "/campaign%06lu.rts",
             (unsigned long)campaignId);

    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
        return false;

    uint8_t buffer[512];
    while (file.available())
    {
        size_t count = file.read(buffer, sizeof(buffer));
        if (count == 0 || stream.write(buffer, count) != count)
        {
            file.close();
            return false;
        }
        delay(0);
    }

    file.close();
    return true;
}

bool Storage::importCSV(const String &csvText, uint32_t &newCampaignId)
{
    newCampaignId = 0;
    if (csvText.length() == 0)
        return false;

    uint32_t sampleCount = 0;
    int position = 0;
    while (position < (int)csvText.length())
    {
        int end = csvText.indexOf('\n', position);
        if (end < 0)
            end = csvText.length();

        String line = csvText.substring(position, end);
        line.trim();
        position = end + 1;

        if (line.length() == 0 || line[0] == '#' || line.startsWith("time_us"))
            continue;

        int comma = line.indexOf(',');
        if (comma <= 0)
            return false;

        String timeText = line.substring(0, comma);
        String forceText = line.substring(comma + 1);
        char *endTime = nullptr;
        char *endForce = nullptr;
        strtoul(timeText.c_str(), &endTime, 10);
        strtof(forceText.c_str(), &endForce);
        if (endTime == timeText.c_str() || endForce == forceText.c_str())
            return false;

        ++sampleCount;
        if (sampleCount > MAX_SAMPLES)
            return false;
    }

    if (sampleCount == 0)
        return false;

    const size_t requiredBytes = sizeof(StorageHeader) +
                                 (size_t)sampleCount * sizeof(Sample);
    if (!ensureSpace(requiredBytes))
        return false;

    const char *tempName = "/import.tmp";
    LittleFS.remove(tempName);
    File file = LittleFS.open(tempName, FILE_WRITE);
    if (!file)
        return false;

    StorageHeader header;
    memset(&header, 0, sizeof(header));
    header.version = STORAGE_HEADER_VERSION;
    header.sampleCount = sampleCount;
    strncpy(header.dateTime, "imported", sizeof(header.dateTime) - 1);
    strncpy(header.description, "Curva importada", sizeof(header.description) - 1);

    if (file.write((const uint8_t *)&header, sizeof(header)) != sizeof(header))
    {
        file.close();
        LittleFS.remove(tempName);
        return false;
    }

    float peak = -FLT_MAX;
    uint32_t writtenSamples = 0;
    position = 0;
    while (position < (int)csvText.length())
    {
        int end = csvText.indexOf('\n', position);
        if (end < 0)
            end = csvText.length();

        String line = csvText.substring(position, end);
        line.trim();
        position = end + 1;

        if (line.startsWith("# datetime,"))
        {
            String value = line.substring(strlen("# datetime,"));
            value.trim();
            value.toCharArray(header.dateTime, sizeof(header.dateTime));
            continue;
        }
        if (line.startsWith("# description,"))
        {
            String value = line.substring(strlen("# description,"));
            value.trim();
            if (value.length() > 50)
                value.remove(50);
            value.toCharArray(header.description, sizeof(header.description));
            continue;
        }
        if (line.length() == 0 || line[0] == '#' || line.startsWith("time_us"))
            continue;

        int comma = line.indexOf(',');
        Sample sample;
        sample.us = (uint32_t)strtoul(line.substring(0, comma).c_str(), nullptr, 10);
        sample.grams = strtof(line.substring(comma + 1).c_str(), nullptr);

        if (sample.grams > peak)
            peak = sample.grams;

        if (file.write((const uint8_t *)&sample, sizeof(sample)) != sizeof(sample))
        {
            file.close();
            LittleFS.remove(tempName);
            return false;
        }

        ++writtenSamples;
        if ((writtenSamples & 0x7F) == 0)
            delay(0);
    }

    if (writtenSamples != sampleCount)
    {
        file.close();
        LittleFS.remove(tempName);
        return false;
    }

    newCampaignId = nextCampaignId();
    header.campaignId = newCampaignId;
    header.peakForce = peak;

    if (!file.seek(0, SeekSet) ||
        file.write((const uint8_t *)&header, sizeof(header)) != sizeof(header))
    {
        file.close();
        LittleFS.remove(tempName);
        newCampaignId = 0;
        return false;
    }

    file.flush();
    file.close();

    char finalName[32];
    snprintf(finalName, sizeof(finalName), "/campaign%06lu.rts",
             (unsigned long)newCampaignId);
    LittleFS.remove(finalName);
    if (!LittleFS.rename(tempName, finalName))
    {
        LittleFS.remove(tempName);
        newCampaignId = 0;
        return false;
    }

    Serial.print("CSV imported as campaign ");
    Serial.println(newCampaignId);
    return true;
}

bool Storage::beginRTSImport(size_t expectedBytes)
{
    cancelRTSImport();

    const size_t maximumBytes = sizeof(StorageHeader) +
                                (size_t)MAX_SAMPLES * sizeof(Sample);
    if (expectedBytes > maximumBytes)
        return false;

    const size_t reserveBytes = expectedBytes > 0 ? expectedBytes : maximumBytes;
    if (!ensureSpace(reserveBytes))
        return false;

    LittleFS.remove("/import.tmp");
    importFile = LittleFS.open("/import.tmp", FILE_WRITE);
    if (!importFile)
        return false;

    importExpectedBytes = expectedBytes;
    importReceivedBytes = 0;
    return true;
}

bool Storage::writeRTSImport(const uint8_t *data, size_t length)
{
    if (!importFile || data == nullptr || length == 0)
        return false;

    const size_t maximumBytes = sizeof(StorageHeader) +
                                (size_t)MAX_SAMPLES * sizeof(Sample);
    if (importReceivedBytes + length > maximumBytes ||
        (importExpectedBytes > 0 && importReceivedBytes + length > importExpectedBytes))
        return false;

    size_t written = importFile.write(data, length);
    importReceivedBytes += written;
    return written == length;
}

bool Storage::finishRTSImport(uint32_t &newCampaignId)
{
    newCampaignId = 0;
    if (!importFile)
        return false;

    importFile.flush();
    importFile.close();

    if (importExpectedBytes > 0 && importReceivedBytes != importExpectedBytes)
    {
        cancelRTSImport();
        return false;
    }

    File file = LittleFS.open("/import.tmp", "r+");
    if (!file)
    {
        cancelRTSImport();
        return false;
    }

    StorageHeader header;
    if (file.read((uint8_t *)&header, sizeof(header)) != sizeof(header) ||
        header.version != STORAGE_HEADER_VERSION ||
        header.sampleCount == 0 ||
        header.sampleCount > MAX_SAMPLES)
    {
        file.close();
        cancelRTSImport();
        return false;
    }

    const size_t expected = sizeof(StorageHeader) +
                            (size_t)header.sampleCount * sizeof(Sample);
    if (expected != importReceivedBytes)
    {
        file.close();
        cancelRTSImport();
        return false;
    }

    newCampaignId = nextCampaignId();
    header.campaignId = newCampaignId;

    if (!file.seek(0, SeekSet) ||
        file.write((const uint8_t *)&header, sizeof(header)) != sizeof(header))
    {
        file.close();
        cancelRTSImport();
        newCampaignId = 0;
        return false;
    }

    file.flush();
    file.close();

    char finalName[32];
    snprintf(finalName, sizeof(finalName), "/campaign%06lu.rts",
             (unsigned long)newCampaignId);
    LittleFS.remove(finalName);
    if (!LittleFS.rename("/import.tmp", finalName))
    {
        cancelRTSImport();
        newCampaignId = 0;
        return false;
    }

    importExpectedBytes = 0;
    importReceivedBytes = 0;
    Serial.print("RTS backup imported as campaign ");
    Serial.println(newCampaignId);
    return true;
}

void Storage::cancelRTSImport()
{
    if (importFile)
        importFile.close();
    LittleFS.remove("/import.tmp");
    importExpectedBytes = 0;
    importReceivedBytes = 0;
}
