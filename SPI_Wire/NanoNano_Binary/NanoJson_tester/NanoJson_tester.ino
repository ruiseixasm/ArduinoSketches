/*
 * ArduinoJson Diagnostic Test
 * Run this standalone sketch to test if ArduinoJson is working
 */

// Include ArduinoJson with PROGMEM support for AVR
#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <ArduinoJson.h>

void setup() {
    Serial.begin(115200);
    delay(3000);  // Give time for Serial Monitor to open
    
    Serial.println("\n\n");
    Serial.println("=========================================");
    Serial.println("      ARDUINOJSON DIAGNOSTIC TEST");
    Serial.println("=========================================");
    
    // Test library version first
    Serial.print("\nArduinoJson Version: ");
    Serial.print(ARDUINOJSON_VERSION_MAJOR);
    Serial.print(".");
    Serial.print(ARDUINOJSON_VERSION_MINOR);
    Serial.print(".");
    Serial.println(ARDUINOJSON_VERSION_REVISION);
    
    Serial.print("ARDUINOJSON_VERSION_MAJOR defined? ");
    #ifdef ARDUINOJSON_VERSION_MAJOR
        Serial.println("YES");
    #else
        Serial.println("NO - Library not properly included!");
    #endif
    
    Serial.println("\n--- Test 1: Empty JSON object ---");
    {
        const char* json = "{}";
        Serial.print("JSON: ");
        Serial.println(json);
        
        #if ARDUINOJSON_VERSION_MAJOR >= 7
            Serial.println("Using ArduinoJson v7+ (JsonDocument)");
            JsonDocument doc;
        #else
            Serial.println("Using ArduinoJson v6 (StaticJsonDocument)");
            StaticJsonDocument<50> doc;
        #endif
        
        DeserializationError error = deserializeJson(doc, json);
        
        Serial.print("Result: ");
        if (error) {
            Serial.print("FAILED - ");
            Serial.println(error.c_str());
        } else {
            Serial.println("SUCCESS");
            Serial.print("Document memory usage: ");
            Serial.print(doc.memoryUsage());
            Serial.println(" bytes");
        }
    }
    
    Serial.println("\n--- Test 2: Simple JSON object ---");
    {
        const char* json = "{\"key\":\"value\"}";
        Serial.print("JSON: ");
        Serial.println(json);
        
        #if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument doc;
        #else
            StaticJsonDocument<100> doc;
        #endif
        
        DeserializationError error = deserializeJson(doc, json);
        
        Serial.print("Result: ");
        if (error) {
            Serial.print("FAILED - ");
            Serial.println(error.c_str());
        } else {
            Serial.println("SUCCESS");
            const char* value = doc["key"];
            Serial.print("Extracted value for 'key': ");
            Serial.println(value);
        }
    }
    
    Serial.println("\n--- Test 3: JSON with numbers ---");
    {
        const char* json = "{\"number\":42,\"float\":3.14}";
        Serial.print("JSON: ");
        Serial.println(json);
        
        #if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument doc;
        #else
            StaticJsonDocument<100> doc;
        #endif
        
        DeserializationError error = deserializeJson(doc, json);
        
        Serial.print("Result: ");
        if (error) {
            Serial.print("FAILED - ");
            Serial.println(error.c_str());
        } else {
            Serial.println("SUCCESS");
            int number = doc["number"];
            float f = doc["float"];
            Serial.print("Number: ");
            Serial.println(number);
            Serial.print("Float: ");
            Serial.println(f, 3);
        }
    }
    
    Serial.println("\n--- Test 4: Your specific JSON ---");
    {
        const char* json = "{\"t\":\"Nano\",\"m\":2,\"n\":\"ON\",\"f\":\"Talker-9f\",\"i\":3540751170,\"c\":24893}";
        Serial.print("JSON: ");
        Serial.println(json);
        Serial.print("JSON length: ");
        Serial.println(strlen(json));
        
        #if ARDUINOJSON_VERSION_MAJOR >= 7
            Serial.println("Using JsonDocument (v7 auto-sizing)");
            JsonDocument doc;
        #else
            Serial.println("Using StaticJsonDocument<256>");
            StaticJsonDocument<256> doc;  // Larger for your JSON
        #endif
        
        DeserializationError error = deserializeJson(doc, json);
        
        Serial.print("Result: ");
        if (error) {
            Serial.print("FAILED - ");
            Serial.println(error.c_str());
            
            // Try with measurement
            #if ARDUINOJSON_VERSION_MAJOR < 7
                size_t needed = measureJson(doc);
                Serial.print("This JSON needs at least: ");
                Serial.print(needed);
                Serial.println(" bytes");
            #endif
        } else {
            Serial.println("SUCCESS");
            const char* t = doc["t"];
            const char* n = doc["n"];
            int m = doc["m"];
            long i = doc["i"];
            
            Serial.print("t: ");
            Serial.println(t);
            Serial.print("n: ");
            Serial.println(n);
            Serial.print("m: ");
            Serial.println(m);
            Serial.print("i: ");
            Serial.println(i);
        }
    }
    
    Serial.println("\n--- Test 5: Serialize JSON ---");
    {
        #if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument doc;
        #else
            StaticJsonDocument<200> doc;
        #endif
        
        doc["message"] = "Hello";
        doc["value"] = 123;
        doc["status"] = true;
        
        Serial.println("Created JSON document:");
        serializeJson(doc, Serial);
        Serial.println();
        
        char buffer[100];
        size_t len = serializeJson(doc, buffer, sizeof(buffer));
        Serial.print("Serialized to buffer (");
        Serial.print(len);
        Serial.print(" bytes): ");
        Serial.println(buffer);
    }
    
    Serial.println("\n--- Memory Test ---");
    {
        #ifdef __AVR__
            extern int __heap_start, *__brkval;
            int v;
            int free_memory = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
            Serial.print("Estimated free RAM: ");
            Serial.print(free_memory);
            Serial.println(" bytes");
        #endif
    }
    
    Serial.println("\n=========================================");
    Serial.println("           DIAGNOSTIC COMPLETE");
    Serial.println("=========================================");
    Serial.println("\nIf any test failed, check:");
    Serial.println("1. ArduinoJson library is installed");
    Serial.println("2. PlatformIO: lib_deps = bblanchon/ArduinoJson");
    Serial.println("3. For AVR: #define ARDUINOJSON_ENABLE_PROGMEM 1");
    Serial.println("4. Memory might be too low");
}

void loop() {
    // Blink LED to show it's running
    static bool ledState = false;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    ledState = !ledState;
    delay(1000);
    
    // Optional: Keep testing in loop
    static unsigned long lastTest = 0;
    if (millis() - lastTest > 5000) {
        lastTest = millis();
        
        #if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument doc;
        #else
            StaticJsonDocument<50> doc;
        #endif
        
        DeserializationError error = deserializeJson(doc, "{\"test\":\"loop\"}");
        if (!error) {
            Serial.print(".");
        }
    }
}
