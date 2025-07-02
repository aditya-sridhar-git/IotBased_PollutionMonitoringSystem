# app.py (Flask backend)
from flask import Flask, render_template, send_from_directory, jsonify
import os

app = Flask(__name__)

# Serve the dashboard
@app.route('/')
def index():
    return render_template('index.html')

# Route to serve the JSON file for ML results
data_folder = os.path.join(app.root_path, 'static')
@app.route('/download-ml-results')
def download_ml_results():
    return send_from_directory(data_folder, 'ml_results.json', as_attachment=True)

# Route to serve JSON content to JS fetch
@app.route('/train-models')
def serve_ml_data():
    with open(os.path.join(data_folder, 'ml_results.json')) as f:
        return jsonify(eval(f.read()))  # Replace eval with json.load for safety

if __name__ == '__main__':
    app.run(debug=True)
