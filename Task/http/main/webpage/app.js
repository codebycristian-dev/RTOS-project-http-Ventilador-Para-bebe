/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;
var wifiConnectInterval = null;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	//getUpdateStatus();
	startDHTSensorInterval();
	$("#connect_wifi").on("click", function(){
		checkCredentials();
	}); 
});   

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() 
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) 
	{		
        var response = JSON.parse(xhr.responseText);
						
	 	document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time

		// If flashing was complete it will return a 1, else -1
		// A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) 
		{
    		// Set the countdown timer time
            seconds = 10;
            // Start the countdown timer
            otaRebootTimer();
        } 
        else if (response.ota_update_status == -1)
		{
            document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

/**
 * Gets DHT22 sensor temperature and humidity values for display on the web page.
 */


function getregValues()
{
	$.getJSON('/read_regs.json', function(data) {
		$("#reg_1").text(data["reg1"]);
		$("#reg_2").text(data["reg2"]);
		$("#reg_3").text(data["reg3"]);
		$("#reg_4").text(data["reg4"]);
		$("#reg_5").text(data["reg5"]);
		$("#reg_6").text(data["reg6"]);
		$("#reg_7").text(data["reg7"]);
		$("#reg_8").text(data["reg8"]);
		$("#reg_9").text(data["reg9"]);
		$("#reg_10").text(data["reg10"]);
	});
}

function getDHTSensorValues()
{
	$.getJSON('/dhtSensor.json', function(data) {
		$("#temperature_reading").text(data["temp"]);
	});
}

/**
 * Sets the interval for getting the updated DHT22 sensor values.
 */

function startDHTSensorInterval()
{
	setInterval(getDHTSensorValues, 5000);    
}


/**
 * Clears the connection status interval.
 */
function stopWifiConnectStatusInterval()
{
	if (wifiConnectInterval != null)
	{
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/**
 * Gets the WiFi connection status.
 */
function getWifiConnectStatus()
{
	var xhr = new XMLHttpRequest();
	var requestURL = "/wifiConnectStatus";
	xhr.open('POST', requestURL, false);
	xhr.send('wifi_connect_status');
	
	if (xhr.readyState == 4 && xhr.status == 200)
	{
		var response = JSON.parse(xhr.responseText);
		
		document.getElementById("wifi_connect_status").innerHTML = "Connecting...";
		
		if (response.wifi_connect_status == 2)
		{
			document.getElementById("wifi_connect_status").innerHTML = "<h4 class='rd'>Failed to Connect. Please check your AP credentials and compatibility</h4>";
			stopWifiConnectStatusInterval();
		}
		else if (response.wifi_connect_status == 3)
		{
			document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>Connection Success!</h4>";
			stopWifiConnectStatusInterval();
		}
	}
}

/**
 * Starts the interval for checking the connection status.
 */
function startWifiConnectStatusInterval()
{
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * Connect WiFi function called using the SSID and password entered into the text fields.
 */
function connectWifi()
{
	// Get the SSID and password
	/*selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	$.ajax({
		url: '/wifiConnect.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		data: {'timestamp': Date.now()}
	});
	*/
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	// Create an object to hold the data to be sent in the request body
	var requestData = {
	  'selectedSSID': selectedSSID,
	  'pwd': pwd,
	  'timestamp': Date.now()
	};
	
	// Serialize the data object to JSON
	var requestDataJSON = JSON.stringify(requestData);
	
	$.ajax({
	  url: '/wifiConnect.json',
	  dataType: 'json',
	  method: 'POST',
	  cache: false,
	  data: requestDataJSON, // Send the JSON data in the request body
	  contentType: 'application/json', // Set the content type to JSON
	  success: function(response) {
		// Handle the success response from the server
		console.log(response);
	  },
	  error: function(xhr, status, error) {
		// Handle errors
		console.error(xhr.responseText);
	  }
	});


	startWifiConnectStatusInterval();
}

/**
 * Checks credentials on connect_wifi button click.
 */
function checkCredentials()
{
	errorList = "";
	credsOk = true;
	
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	if (selectedSSID == "")
	{
		errorList += "<h4 class='rd'>SSID cannot be empty!</h4>";
		credsOk = false;
	}
	if (pwd == "")
	{
		errorList += "<h4 class='rd'>Password cannot be empty!</h4>";
		credsOk = false;
	}
	
	if (credsOk == false)
	{
		$("#wifi_connect_credentials_errors").html(errorList);
	}
	else
	{
		$("#wifi_connect_credentials_errors").html("");
		connectWifi();    
	}
}

/**
 * Shows the WiFi password if the box is checked.
 */
function showPassword()
{
	var x = document.getElementById("connect_pass");
	if (x.type === "password")
	{
		x.type = "text";
	}
	else
	{
		x.type = "password";
	}
}


function send_register()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();
    hours = $("#hours").val();
    minutes = $("#minutes").val();
    
    // Create an array for selected days
    var selectedDays = [];
    if ($("#day_mon").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_tue").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_wed").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_thu").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_fri").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_sat").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_sun").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");

    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'hours': hours,
        'minutes': minutes,
        'selectedDays': selectedDays,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regchange.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });

    // Print the resulting JSON to the console (for testing)
    //console.log(requestDataJSON);
}

/**
 * toogle led function.
 */
function read_reg()
{

	
	$.ajax({
		url: '/readreg.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		//headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		//data: {'timestamp': Date.now()}
	});
//	var xhr = new XMLHttpRequest();
//	xhr.open("POST", "/toogle_led.json");
//	xhr.setRequestHeader("Content-Type", "application/json");
//	xhr.send(JSON.stringify({data: "mi información"}));
}


function erase_register()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();



    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regchange.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });

    // Print the resulting JSON to the console (for testing)
    //console.log(requestDataJSON);
}

function toogle_led() 
{	
	$.ajax({
		url: '/toogle_led.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

function brigthness_up() 
{	
	$.ajax({
		url: '/toogle_led.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}
/* ============================================================
   VENTILADOR INTELIGENTE - APP.JS (Opción B estilo tarjetas)
   ============================================================ */

let fanStateInterval = null;

// ============================================================
// ACTUALIZAR UI SEGÚN MODO
// ============================================================
function updateFanModeUI(mode) {
	const modes = ["mode_manual", "mode_auto", "mode_program"];

	modes.forEach((id, index) => {
		let btn = document.getElementById(id);
		if (index === mode) {
			btn.classList.add("active");
		} else {
			btn.classList.remove("active");
		}
	});

	// Mostrar/Ocultar tarjetas según modo
	document.getElementById("card_manual").style.display =
		mode === 0 ? "block" : "none";
	document.getElementById("card_auto").style.display =
		mode === 1 ? "block" : "none";
	document.getElementById("card_program").style.display =
		mode === 2 ? "block" : "none";
}

// ============================================================
// OBTENER ESTADO DEL VENTILADOR
// ============================================================
let autoLocalTmin = null;
let autoLocalTmax = null;
function loadFanState() {
	fetch("/fan/get_state.json")
		.then(response => response.json())
		.then(data => {

			// Datos rápidos
			document.getElementById("fan_temp").innerText = data.temperature.toFixed(1);
			document.getElementById("fan_presence").innerText = data.presence ? "Sí" : "No";
			document.getElementById("fan_pwm").innerText = data.pwm;

			// Modo
			updateFanModeUI(data.mode);

			// Auto config
			if (autoLocalTmin === null) {
				document.getElementById("auto_tmin").value = data.Tmin;
			}

			// Solo actualizar Tmax si NO hay un valor local pendiente
			if (autoLocalTmax === null) {
				document.getElementById("auto_tmax").value = data.Tmax;
			}


		})
		.catch(err => console.log("Error obteniendo estado:", err));
}

// ============================================================
// CAMBIAR MODO DEL VENTILADOR
// ============================================================
function setFanMode(mode) {
	fetch("/fan/set_mode.json", {
		method: "POST",
		body: mode.toString(),
	})
		.then(() => {
			console.log("Modo cambiado:", mode);
			loadFanState();
		})
		.catch(err => console.log("Error cambiando modo:", err));
}

// ============================================================
// MODO MANUAL: SLIDER
// ============================================================
function updateManualLabel() {
	let val = document.getElementById("manual_pwm").value;
	document.getElementById("manual_pwm_label").innerText = val;
}

function saveManualPWM() {
	let value = document.getElementById("manual_pwm").value;

	fetch("/fan/set_manual_pwm.json", {
		method: "POST",
		body: value.toString(),
	})
		.then(() => {
			console.log("PWM manual guardado:", value);
			loadFanState();
		});
}

// ============================================================
// MODO AUTOMÁTICO (Tmin / Tmax)
// ============================================================

function saveAutoConfig() {
	let Tmin_str = document.getElementById("auto_tmin").value;
	let Tmax_str = document.getElementById("auto_tmax").value;

	// Validación: campos no pueden estar vacíos
	if (Tmin_str.trim() === "" || Tmax_str.trim() === "") {
		alert("Debes ingresar valores válidos de Tmin y Tmax.");
		return;
	}

	let Tmin = parseFloat(Tmin_str);
	let Tmax = parseFloat(Tmax_str);

	// Validación numérica
	if (isNaN(Tmin) || isNaN(Tmax)) {
		alert("Los valores deben ser numéricos.");
		return;
	}

	// Validación lógica
	if (Tmin >= Tmax) {
		alert("Tmin debe ser menor que Tmax.");
		return;
	}

	let json = JSON.stringify({ Tmin: Tmin, Tmax: Tmax });

	fetch("/fan/set_auto.json", {
		method: "POST",
		headers: {"Content-Type": "application/json"},
		body: json
	})
		.then(() => {
			console.log("Auto-config guardada");
			autoLocalTmin = null;
			autoLocalTmax = null;	
			loadFanState();
		});
}


// ============================================================
// REGISTROS PROGRAMADOS
// ============================================================
function createRegisterCard(id, reg) {
	return `
    <div class="card inner-card">
        <h4>Registro ${id + 1}</h4>

        <label>Activo:
            <input type="checkbox" id="reg${id}_active" ${reg.active ? "checked" : ""}>
        </label>

        <label>Hora inicio:</label>
        <input type="time" id="reg${id}_start" value="${reg.hour_start.toString().padStart(2, "0")}:${reg.min_start.toString().padStart(2, "0")}">

        <label>Hora fin:</label>
        <input type="time" id="reg${id}_end" value="${reg.hour_end.toString().padStart(2, "0")}:${reg.min_end.toString().padStart(2, "0")}">

        <label>T0% (°C):</label>
        <input type="number" id="reg${id}_t0" step="0.1" value="${reg.temp0}">

        <label>T100% (°C):</label>
        <input type="number" id="reg${id}_t100" step="0.1" value="${reg.temp100}">

        <button class="btn primary" onclick="saveRegister(${id})">Guardar</button>
    </div>
    `;
}

function loadRegisters() {
	let container = document.getElementById("program_list");
	container.innerHTML = "";

	for (let i = 0; i < 3; i++) {
		fetch(`/fan/get_register.json?id=${i}`)
			.then(res => res.json())
			.then(reg => {
				container.innerHTML += createRegisterCard(i, reg);
			});
	}
}

function saveRegister(id) {
	let active = document.getElementById(`reg${id}_active`).checked ? 1 : 0;

	let [hs, ms] = document.getElementById(`reg${id}_start`).value.split(":");
	let [he, me] = document.getElementById(`reg${id}_end`).value.split(":");

	let t0 = document.getElementById(`reg${id}_t0`).value;
	let t100 = document.getElementById(`reg${id}_t100`).value;

	let json = JSON.stringify({
		id: id,
		active: active,
		hour_start: parseInt(hs),
		min_start: parseInt(ms),
		hour_end: parseInt(he),
		min_end: parseInt(me),
		temp0: parseFloat(t0),
		temp100: parseFloat(t100)
	});

	fetch("/fan/set_register.json", {
		method: "POST",
		body: json
	}).then(() => {
		console.log("Registro guardado:", id);
	});
}

// ============================================================
// INICIO AUTOMÁTICO
// ============================================================
function startFanStateInterval() {
	if (fanStateInterval) clearInterval(fanStateInterval);

	fanStateInterval = setInterval(loadFanState, 1000);
}

// Llamamos al iniciar la página
window.addEventListener("load", () => {
	loadFanState();
	startFanStateInterval();
	loadRegisters();
});











    










    


