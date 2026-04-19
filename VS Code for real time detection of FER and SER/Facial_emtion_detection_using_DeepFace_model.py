from deepface import DeepFace
import cv2

# open laptop webcam
cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()

    if not ret:
        break

    # resize for faster processing
    frame = cv2.resize(frame, (640,480))

    try:
        result = DeepFace.analyze(frame, actions=['emotion'], enforce_detection=False)
        emotion = result[0]['dominant_emotion']
    except:
        emotion = "No face"

    # display emotion on screen
    cv2.putText(frame, emotion, (50,50),
                cv2.FONT_HERSHEY_SIMPLEX,
                1, (0,255,0), 2)

    cv2.imshow("Emotion Detection", frame)

    # press ESC to exit
    if cv2.waitKey(1) == 27:
        break

cap.release()
cv2.destroyAllWindows()