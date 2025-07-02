from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
from sklearn.ensemble import RandomForestRegressor
import pandas as pd
import numpy as np
import requests

app = Flask(__name__)
CORS(app)

models = {}

# ThingSpeak channel data URL
THINGSPEAK_URL = "https://api.thingspeak.com/channels/2965771/feeds.json?api_key=I6D9WGROEFOLZJIE"

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/train-models', methods=['GET'])
def train_models():
    global models

    # Fetch data from ThingSpeak
    response = requests.get(THINGSPEAK_URL)
    data = response.json()
    feeds = data['feeds']
    df = pd.DataFrame(feeds)

    # Fields for sensors: PM1.0, PM2.5, PM10, Loudness
    fields = ['field1', 'field2', 'field3', 'field6']
    models = {}

    # Convert sensor fields to numeric
    for f in fields:
        df[f] = pd.to_numeric(df[f], errors='coerce')

    df.dropna(inplace=True)  # Remove rows with NaNs

    # Train RandomForest for each field
    for f in fields:
        x = np.array(range(len(df))).reshape(-1, 1)  # time axis
        y = df[f].values
        model = RandomForestRegressor(n_estimators=100, random_state=42)
        model.fit(x, y)
        pred = model.predict(x)
        models[f] = {
            "model": model,
            "original": list(range(len(df))),
            "actual": y.tolist(),
            "predicted": pred.tolist(),
            "accuracy": model.score(x, y),
            "latest": y[-1] if len(y) > 0 else None
        }

    # Return predictions and accuracy to frontend
    return jsonify({f: {
        "original": models[f]["original"],
        "actual": models[f]["actual"],
        "predicted": models[f]["predicted"],
        "accuracy": models[f]["accuracy"],
        "latest": models[f]["latest"]
    } for f in models})


if __name__ == '__main__':
    app.run(debug=True)
