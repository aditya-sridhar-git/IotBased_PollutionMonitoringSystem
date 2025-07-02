const channelId = '2965771';
const readApiKey = 'I6D9WGROEFOLZJIE';

// Fetch single sensor value from ThingSpeak
async function fetchThingSpeak(fieldId) {
    const url = `https://api.thingspeak.com/channels/${channelId}/fields/${fieldId}.json?api_key=${readApiKey}&results=1`;
    const res = await fetch(url);
    const data = await res.json();
    return data.feeds[0][`field${fieldId}`];
}

// Update all live sensor values
async function updateRealTimeData() {
    document.getElementById("mq135_ppb").textContent = await fetchThingSpeak(1);
    document.getElementById("mq7_ppb").textContent = await fetchThingSpeak(2);
    document.getElementById("mq2_ppb").textContent = await fetchThingSpeak(3);
    document.getElementById("dB").textContent = await fetchThingSpeak(4);
    document.getElementById("pm25").textContent = await fetchThingSpeak(5);
    document.getElementById("pm10").textContent = await fetchThingSpeak(6);
    document.getElementById("Temperature").textContent = await fetchThingSpeak(7);
    document.getElementById("Humidity").textContent = await fetchThingSpeak(8);
}

// Handle tab switching between sensor sections
function showPage(id, btn) {
    document.querySelectorAll('.sensor-page').forEach(p => p.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    document.querySelectorAll('.nav-pill').forEach(p => p.classList.remove('active'));
    btn.classList.add('active');
}

// Fetch ML data and render graphs
async function fetchMLDataAndRenderCharts() {
    try {
        const res = await fetch('/train-models');
        const data = await res.json();

        if (data.field4) drawChart("chart_loudness", data.field4, "Loudness (dB)");
        if (data.field5) drawChart("chart_pm25", data.field5, "PM2.5");
        if (data.field6) drawChart("chart_pm10", data.field6, "PM10");
    } catch (err) {
        console.error("Error fetching ML data:", err);
    }
}

// Render chart using Chart.js
function drawChart(canvasId, data, label) {
    const ctx = document.getElementById(canvasId).getContext("2d");
    new Chart(ctx, {
        type: 'line',
        data: {
            labels: data.original,
            datasets: [
                {
                    label: 'Actual',
                    data: data.actual,
                    borderColor: 'green',
                    borderWidth: 2,
                    fill: false
                },
                {
                    label: 'Predicted',
                    data: data.predicted,
                    borderColor: 'orange',
                    borderDash: [5, 5],
                    borderWidth: 2,
                    fill: false
                }
            ]
        },
        options: {
            plugins: {
                title: {
                    display: true,
                    text: `${label} Prediction (Accuracy: ${(data.accuracy * 100).toFixed(2)}%)`
                }
            },
            responsive: true
        }
    });
}

// Initial calls
setInterval(updateRealTimeData, 5000);
updateRealTimeData();
fetchMLDataAndRenderCharts();
