import asyncio
import json
import time
import requests
from bleak import BleakScanner, BleakClient

# --- CONFIGURATION ---
DEVICE_NAME = "UNO_R4_EDGE"
CHAR_UUID = "00002a56-0000-1000-8000-00805f9b34fb"
THINGSPEAK_API_KEY = "2QYCUFEO85PDVP9G"
CHANNEL_ID = "3224503"

state = {
    "suspicion_score": 0.0,
    "is_running": True,
    "last_command_time": 0,
    "last_upload_time": 0,
    "emergency_sent": False
}

def upload_to_thingspeak(payload, score):
    """Sends data to ThingSpeak and prints the result to your terminal"""
    now = time.time()
    
    # ThingSpeak Free limit is 15 seconds. We use 16 to be safe.
    if now - state["last_upload_time"] < 16:
        return

    # Garage Status logic (Field 6)
    dist = payload.get('dist', 999)
    garage_status = 1 if dist <= 5 else 0

    url = "https://api.thingspeak.com/update"
    params = {
        "api_key": THINGSPEAK_API_KEY,
        "field1": payload.get('t'),
        "field2": payload.get('h'),
        "field3": payload.get('g'),
        "field4": payload.get('f'),
        "field5": score,
        "field6": garage_status
    }
    
    try:
        # We increase timeout slightly to ensure the request finishes
        r = requests.get(url, params=params, timeout=5)
        if r.status_code == 200 and r.text != "0":
            print(f"\n☁️ ThingSpeak Update Success! Entry ID: {r.text}")
            state["last_upload_time"] = now
        else:
            print(f"\n☁️ ThingSpeak Rejected (Wait 15s): {r.text}")
    except Exception as e:
        print(f"\n☁️ Cloud Error: {e}")

async def run_system():
    print(f"--- AI Brain Online: Gradual Mode ---")
    print(f"--- Uploading to Channel: {CHANNEL_ID} ---")
    
    while state["is_running"]:
        try:
            device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
            if not device: continue

            async with BleakClient(device) as client:
                print(f"✅ Connected to Arduino")
                await asyncio.sleep(1.0)

                async def notification_handler(characteristic, data):
                    try:
                        raw_str = data.decode().strip()
                        if not raw_str.startswith('{'): return
                        
                        payload = json.loads(raw_str)
                        f_raw = payload.get('f', 1024)

                      
                        if f_raw < 500:
                            state["suspicion_score"] += 2.0
                        else:
                            state["suspicion_score"] = 0 

                        state["suspicion_score"] = max(0, min(state["suspicion_score"], 50))
                        
                        # Upload attempt
                        upload_to_thingspeak(payload, state["suspicion_score"])
                        
                    except: pass

                await client.start_notify(CHAR_UUID, notification_handler)
                
                while client.is_connected:
                    current_score = state["suspicion_score"]
                    now = time.time()
                    
                    
                    print(f"Monitoring... Score: {round(current_score, 1)}", end="\r")

                    if current_score >= 15.0:
                        if now - state["last_command_time"] > 3.0:
                            print(f"\n🚨 FIRE ALERT (Score: {current_score}) - Pulsing Arduino")
                            await client.write_gatt_char(CHAR_UUID, b"FIRE")
                            state["last_command_time"] = now
                            state["emergency_sent"] = True
                        await asyncio.sleep(0.5)
                    
                    elif current_score <= 0 and state["emergency_sent"]:
                        print("\n✅ SYSTEM SAFE - Resetting Alarms")
                        await client.write_gatt_char(CHAR_UUID, b"SAFE")
                        state["emergency_sent"] = False
                        state["last_command_time"] = 0 
                        await asyncio.sleep(1.0) 
                    
                    else:
                        await asyncio.sleep(0.1)

        except Exception as e:
            print(f"\n⚠️ Connection Error: {e}")
            await asyncio.sleep(2)

if __name__ == "__main__":
    try:
        asyncio.run(run_system())
    except KeyboardInterrupt:
        print("\nShutdown.")
