import speech_recognition as sr

r = sr.Recognizer()
r.energy_threshold = 300  # helps with noise
r.dynamic_energy_threshold = True

mic = sr.Microphone()

print("🎤 Speech Recognition started (Ctrl+C to stop)")

with mic as source:
    r.adjust_for_ambient_noise(source, duration=1)

while True:
    try:
        with mic as source:
            print("Listening...")
            audio = r.listen(source, timeout=5)

        text = r.recognize_google(audio)
        print("You said:", text)

    except sr.UnknownValueError:
        print("❌ Could not understand audio, try again")

    except sr.WaitTimeoutError:
        print("⌛ No speech detected")

    except KeyboardInterrupt:
        print("\n🛑 Stopped by user")
        break

    except Exception as e:
        print("⚠️ Error:", e)
