const channelId = '2965771';
const readApiKey = 'I6D9WGROEFOLZJIE';

async function fetchThingSpeak(fieldId) {
    const url = `https://api.thingspeak.com/channels/${channelId}/fields/${fieldId}.json?api_key=${readApiKey}&results=1`;
    const res = await fetch(url);
    const data = await res.json();
    return data.feeds[0][`field${fieldId}`];
}

async function updateRealTimeData() {
    // Real sensor values from ThingSpeak
    document.getElementById("dB").textContent = await fetchThingSpeak(6);     // Loudness
    document.getElementById("pm1").textContent = await fetchThingSpeak(1);    // PM1.0
    document.getElementById("pm25").textContent = await fetchThingSpeak(2);   // PM2.5
    document.getElementById("pm10").textContent = await fetchThingSpeak(3);   // PM10
    document.getElementById("mq7_ppb").textContent = await fetchThingSpeak(4);  // MQ7
    document.getElementById("mq2_ppb").textContent = await fetchThingSpeak(5);  // MQ2
    document.getElementById("mq135_ppb").textContent = await fetchThingSpeak(7); // MQ135
}

function showPage(id, btn) {
    document.querySelectorAll('.sensor-page').forEach(p => p.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    document.querySelectorAll('.nav-pill').forEach(p => p.classList.remove('active'));
    btn.classList.add('active');
}

async function fetchMLDataAndRenderCharts() {
    const res = await fetch('/train-models');
    const data = await res.json();

    if (data.field6) drawChart("chart_loudness", data.field6, "Loudness (dB)");
    if (data.field1) drawChart("chart_pm1", data.field1, "PM1.0");
    if (data.field2) drawChart("chart_pm25", data.field2, "PM2.5");
    if (data.field3) drawChart("chart_pm10", data.field3, "PM10");
}

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

// Update data and graphs on load and every 5 seconds
setInterval(updateRealTimeData, 5000);
updateRealTimeData();
fetchMLDataAndRenderCharts();
