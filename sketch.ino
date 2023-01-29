#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define DHTPIN 8
#define DHTTYPE DHT22
#define BUTTON_PIN 4

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float CWU_AUTO_START_TEMP; // lower than CWU_AUTO_STOP_TEMP
float CWU_AUTO_STOP_TEMP;  // higher than CWU_AUTO_START_TEMP
bool CWU_STATUS;
bool CO_STATUS;
bool DIRECTIONAL_VALVE_STATUS; // TRUE - CWU, FALSE - CO
bool IS_ADMIN;
float LAST_TEAMPERATURE_READ;

void setCwuAutoStartTemp(int temp)
{
    CWU_AUTO_START_TEMP = temp;
}

void setCwuAutoStopTemp(int temp)
{
    CWU_AUTO_STOP_TEMP = temp;
}

void setCwuStatus(bool status)
{
    CWU_STATUS = status;

    if (status)
        digitalWrite(13, HIGH);
    else
        digitalWrite(13, LOW);
}

void setCoStatus(bool status)
{
    CO_STATUS = status;

    if (status)
        digitalWrite(12, HIGH);
    else
        digitalWrite(12, LOW);
}

void set3DStatus(bool status)
{
    DIRECTIONAL_VALVE_STATUS = status;
    if (status)
        digitalWrite(2, HIGH);
    else
        digitalWrite(2, LOW);
}

void setAdminStatus(bool status)
{
    IS_ADMIN = status;
}

void getCurrentCwuAutoStartTemp()
{
    Serial.println("CWU auto start temp: " + String(CWU_AUTO_START_TEMP));
}

void getCurrentCwuAutoStopTemp()
{
    Serial.println("CWU auto stop temp: " + String(CWU_AUTO_STOP_TEMP));
}

float getCurrentWaterTankTemp()
{
    return dht.readTemperature();
}

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    dht.begin();
    pinMode(2, OUTPUT);  // 3D
    pinMode(12, OUTPUT); // rel21 - CO
    pinMode(13, OUTPUT); // rel22 - CWU
    pinMode(3, OUTPUT);  // rel23 - 50% power mode - summer mode
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // default values
    setAdminStatus(false);
    setSummerMode(false);
    set3DStatus(true);
    setCwuAutoStartTemp(42);
    setCwuAutoStopTemp(46);
}

void updateStatuses(float current_temp)
{
    if (current_temp < CWU_AUTO_START_TEMP) // CWU start and CO stop
    {
        if (!CWU_STATUS)
            setCwuStatus(true);

        if (CO_STATUS)
            setCoStatus(false);

        if (!DIRECTIONAL_VALVE_STATUS)
            setCoStatus(true);
    }
    else if (current_temp < CWU_AUTO_STOP_TEMP) // CWU stop and CO start
    {
        if (CWU_STATUS)
            setCwuStatus(false);

        if (!CO_STATUS)
            setCoStatus(true);

        if (!DIRECTIONAL_VALVE_STATUS)
            set3DStatus(true);
    }
    else // CWU start and CO start
    {
        if (!CWU_STATUS)
            setCwuStatus(true);

        if (!CO_STATUS)
            setCoStatus(true);

        if (DIRECTIONAL_VALVE_STATUS)
            set3DStatus(false);
    }
}

bool calculateStatus(float current_temp)
{
    if (current_temp != LAST_TEAMPERATURE_READ)
    {
        LAST_TEAMPERATURE_READ = current_temp;
        return true;
    }

    return false;
}

void displayCurrentStatus(float current_temp)
{
    Serial.println("================================");
    Serial.println("CWU: " + String(CWU_STATUS));
    Serial.println("CO: " + String(CO_STATUS));

    if (DIRECTIONAL_VALVE_STATUS)
        Serial.println("3D: CWU");
    else
        Serial.println("3D: CO");

    Serial.println("CWU auto start temp: " + String(CWU_AUTO_START_TEMP));
    Serial.println("CWU auto stop temp: " + String(CWU_AUTO_STOP_TEMP));
    Serial.println("Current water tank temp: " + String(current_temp));
}

char **parseCommand(char *buffer)
{
    char **command_table = malloc(2 * sizeof(char *));
    char *word = strtok(buffer, " \n\t");
    int index = 0;

    while (true)
    {
        if (word == NULL)
            break;

        command_table[index] = word;
        index++;
        word = strtok(NULL, " \n\t");
        command_table[index] = NULL;
    }

    return command_table;
}

int convertStringToInt(char firstDigit, char secondDigit)
{
    int converteFirsDigit = firstDigit - '0';
    int convertedSecondDigit = secondDigit - '0';

    return converteFirsDigit * 10 + convertedSecondDigit;
}

void handleCommands(String command)
{
    char buffer[command.length() + 1];
    command.toCharArray(buffer, buffer);
    char **command_tab = parseCommand(buffer);

    if (strcmp(command_tab[0], "reset") == 0 &&
        command_tab[1] == NULL)
    {
        setAdminStatus(false);
        Serial.println("Loaded default settings");
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "start") == 0 &&
             command_tab[2] == NULL)
    {
        setCwuStatus(true);
        setAdminStatus(true);

        Serial.println("CWU started");
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "stop") == 0 &&
             command_tab[2] == NULL)
    {
        setCwuStatus(false);
        setAdminStatus(true);

        Serial.println("CWU stopped");
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "temp") == 0 &&
             command_tab[2] == NULL)
    {
        Serial.println("Current water tank temp: " + String(getCurrentWaterTankTemp()));
    }
    else if (strcmp(command_tab[0], "heating") == 0 &&
             strcmp(command_tab[1], "start") == 0 &&
             command_tab[2] == NULL)
    {
        setCoStatus(true);
        setAdminStatus(true);

        Serial.println("CO started");
    }
    else if (strcmp(command_tab[0], "heating") == 0 &&
             strcmp(command_tab[1], "stop") == 0 &&
             command_tab[2] == NULL)
    {
        setCoStatus(false);
        setAdminStatus(true);

        Serial.println("CO stopped");
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "start") == 0 &&
             strcmp(command_tab[2], "temp") == 0 &&
             command_tab[3] != NULL)
    {
        int start_temp = convertStringToInt(command_tab[3][0], command_tab[3][1]);

        if (start_temp > CWU_AUTO_STOP_TEMP)
        {
            Serial.println("ERROR: CWU start temp must be less than CWU stop temp");
            return;
        }

        setCwuAutoStartTemp(start_temp);

        Serial.println("CWU start temp set to " + String(start_temp));
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "stop") == 0 &&
             strcmp(command_tab[2], "temp") == 0 &&
             command_tab[3] != NULL)
    {
        int stop_temp = convertStringToInt(command_tab[3][0], command_tab[3][1]);

        if (stop_temp < CWU_AUTO_START_TEMP)
        {
            Serial.println("ERROR: CWU stop temp must be greater than CWU start temp");
            return;
        }

        setCwuAutoStopTemp(stop_temp);

        Serial.println("CWU stop temp set to " + String(stop_temp));
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "stop") == 0 &&
             strcmp(command_tab[2], "temp") == 0 &&
             command_tab[3] == NULL)
    {
        getCurrentCwuAutoStopTemp();
    }
    else if (strcmp(command_tab[0], "hotwater") == 0 &&
             strcmp(command_tab[1], "start") == 0 &&
             strcmp(command_tab[2], "temp") == 0 &&
             command_tab[3] == NULL)
    {
        getCurrentCwuAutoStartTemp();
    }
    else
    {
        Serial.println("ERROR: Invalid command");
    }
}

void loop()
{
    float current_temperature = getCurrentWaterTankTemp();

    String potential_command = Serial.readString();
    bool lastAdminStatus = IS_ADMIN;

    if (potential_command != "")
        handleCommands(potential_command);

    if (lastAdminStatus != IS_ADMIN)
        displayCurrentStatus(current_temperature);

    if ((!IS_ADMIN && calculateStatus(current_temperature)) || (lastAdminStatus && !IS_ADMIN))
    {
        updateStatuses(current_temperature);
        displayCurrentStatus(current_temperature);
    }
}
