from flask import Flask, request, jsonify, render_template
import json
import requests

app = Flask(__name__)

data = {}
Tdata = {}

@app.route('/', methods=['GET','POST','PUT'])
def home():
    global data
    global Tdata

    if request.method == 'PUT':
        json_data = request.get_json()
        thermostat_temperature = None

        if json_data is None:
            try:
                with open('temp.json', 'r') as file:
                    Tdata = json.load(file)
                    thermostat_temperature = Tdata.get('Thermostat Temperature')
                    if thermostat_temperature is not None:
                        # Update the data with the new thermostat temperature
                        Tdata['Thermostat Temperature'] = thermostat_temperature

                       #Save the updated data to temp.json
                        with open('temp.json', 'w') as file:
                            json.dump(Tdata, file)
            except FileNotFoundError:
                pass
        else:
            thermostat_temperature = json_data.get('Thermostat Temperature')
            if thermostat_temperature is not None:
                # Load existing data from temp.json (if it exists)
                try:
                    with open('temp.json', 'r') as file:
                        Tdata = json.load(file)
                except FileNotFoundError:
                    Tdata = {}

                # Update the data with the new thermostat temperature
                Tdata['Thermostat Temperature'] = thermostat_temperature

                # Save the updated data to temp.json
                with open('temp.json', 'w') as file:
                    json.dump(Tdata, file)

        response_data = {
            'Thermostat Temperature': thermostat_temperature
        }
        return jsonify(response_data)

    if request.method == 'POST':
        if request.headers['Content-Type'] == 'application/json':
            data = request.get_json()  # Get the JSON data from the request
            # Save the JSON data to a file
            with open('data.json', 'w') as file:
                json.dump(data, file)

            return jsonify(message='Data received and saved successfully')
        else:
            return jsonify(error='Invalid Content-Type. Expected application/json.')

    if request.method == 'GET':
        halllight = data.get('Sensor1')
        value = data.get('Value1')
        gasLevel = data.get('GasAnalogValue')
        gasSafety = data.get('GasSafety')
        homeTemp = round(data.get('Home Temperature'), 1) if data.get('Home Temperature') else None
        ACState = data.get('Air Conditioning')
        Heating = data.get('Heating')
        doorSensor = data.get('Door Sensor')


        return render_template('data.html', doorSensor = doorSensor, halllight=halllight, value=value, gasLevel=gasLevel, gasSafety=gasSafety, homeTemp = homeTemp,
                               ACState = ACState, Heating = Heating)

    return jsonify(message='Only GET and POST requests are allowed for this endpoint')

if __name__ == "__main__":
    app.run(host='0.0.0.0', debug=True)
