import numpy as np
import librosa
import tensorflow as tf
import pickle
import speech_recognition as sr
import io

model = tf.keras.models.load_model(r"C:\Users\Ann\Desktop\Uni\FYP\speech_emotion_model.h5", compile=False)

with open(r"C:\Users\Ann\Desktop\Uni\FYP\label_encoder.pkl", "rb") as f:
    le = pickle.load(f)

# Config
SAMPLE_RATE = 16000
N_MELS = 64
HOP_LENGTH = 256
TARGET_FRAMES = 188


# 2. Setup Recognition
r = sr.Recognizer()
r.energy_threshold = 300 
r.dynamic_energy_threshold = True
mic = sr.Microphone(sample_rate=SAMPLE_RATE)

# --- START GREETING ---
print("\n🤖 AI: Hello! How are you doing today?")
print("(Waiting for your answer before starting the main loop...)\n")

with mic as source:
    r.adjust_for_ambient_noise(source, duration=1)
    try:
        audio_intro = r.listen(source, timeout=10)
        intro_text = r.recognize_google(audio_intro)
        print(f"👤 You: {intro_text}")
        print("🤖 AI: I see! Let me start monitoring your emotions now.\n")
    except Exception:
        print("🤖 AI: I didn't catch that, but I'll start listening now anyway!\n")
# --- END GREETING ---

print("🎤 System Ready: Monitoring Speech & Emotion (Ctrl+C to stop)")


try:
    while True:
        with mic as source:
            print("\nListening...")
            try:
                audio_data = r.listen(source, timeout=5, phrase_time_limit=5)
                
                # --- SPEECH TO TEXT ---
                try:
                    text = r.recognize_google(audio_data)
                    print(f"🗨️  You said: {text}")
                except sr.UnknownValueError:
                    print("🗨️  Speech: (Could not understand words)")
                
                # --- EMOTION DETECTION ---
                wav_data = audio_data.get_wav_data()
                audio_np, _ = librosa.load(io.BytesIO(wav_data), sr=SAMPLE_RATE)

                mel_spec = librosa.feature.melspectrogram(
                    y=audio_np, 
                    sr=SAMPLE_RATE, 
                    n_mels=N_MELS, 
                    hop_length=HOP_LENGTH
                )
                log_mel_spec = librosa.power_to_db(mel_spec, ref=np.max)
                log_mel_spec = (log_mel_spec - np.mean(log_mel_spec)) / (np.std(log_mel_spec) + 1e-6)

                if log_mel_spec.shape[1] < TARGET_FRAMES:
                    pad = TARGET_FRAMES - log_mel_spec.shape[1]
                    log_mel_spec = np.pad(log_mel_spec, ((0,0),(0,pad)))
                else:
                    log_mel_spec = log_mel_spec[:,:TARGET_FRAMES]

                input_data = log_mel_spec[np.newaxis, ..., np.newaxis]
                prediction = model.predict(input_data, verbose=0)
                emotion_index = np.argmax(prediction)
                emotion = le.inverse_transform([emotion_index])[0]
                confidence = np.max(prediction)

                print(f"🎭 Emotion: {emotion.upper()} ({confidence*100:.2f}%)")

            except sr.WaitTimeoutError:
                continue 

except KeyboardInterrupt:
    print("\n🛑 Stopped by user")