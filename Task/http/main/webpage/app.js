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
			if (mode === 2) {
				loadRegisters();
			}
			if (mode === 0) {
				saveManualPWM();
			}
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

	// Traducción bitmask → checked
	function check(mask, bit) {
		return (mask & (1 << bit)) ? "checked" : "";
	}

	return `
    <div class="card inner-card" id="reg_card_${id}">
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

        <label>Días:</label>
        <div class="days small-days">
            <label><input type="checkbox" id="reg${id}_day0" ${check(reg.days, 0)}>Lun</label>
            <label><input type="checkbox" id="reg${id}_day1" ${check(reg.days, 1)}>Mar</label>
            <label><input type="checkbox" id="reg${id}_day2" ${check(reg.days, 2)}>Mié</label>
            <label><input type="checkbox" id="reg${id}_day3" ${check(reg.days, 3)}>Jue</label>
            <label><input type="checkbox" id="reg${id}_day4" ${check(reg.days, 4)}>Vie</label>
            <label><input type="checkbox" id="reg${id}_day5" ${check(reg.days, 5)}>Sáb</label>
            <label><input type="checkbox" id="reg${id}_day6" ${check(reg.days, 6)}>Dom</label>
        </div>

        <button class="btn primary" onclick="saveRegister(${id})">Guardar</button>
    </div>
    `;
}

function loadRegisters() {
	let container = document.getElementById("program_blocks");
	container.innerHTML = ""; // limpiar contenido

	for (let i = 0; i < 3; i++) {
		const visibleId = i + 1; // IDs visibles: 0, 1, 2
		fetch(`/fan/get_register.json?id=${visibleId}`)
			.then(res => res.json())
			.then(reg => {
				console.log("Registro recibido:", reg);
				container.innerHTML += createRegisterCard(i, reg);
			})
			.catch(err => console.error("Error leyendo registro:", err));
	}
}

function saveRegister(id) {

	function getDaysMask(id) {
		let mask = 0;
		for (let d = 0; d < 7; d++) {
			if (document.getElementById(`reg${id}_day${d}`).checked)
				mask |= (1 << d);
		}
		return mask;
	}

	let active = document.getElementById(`reg${id}_active`).checked ? 1 : 0;

	let [hs, ms] = document.getElementById(`reg${id}_start`).value.split(":");
	let [he, me] = document.getElementById(`reg${id}_end`).value.split(":");

	let t0 = parseFloat(document.getElementById(`reg${id}_t0`).value);
	let t100 = parseFloat(document.getElementById(`reg${id}_t100`).value);

	let days = getDaysMask(id);

	let json = JSON.stringify({
		id: id + 1,
		active: active,
		hour_start: parseInt(hs),
		min_start: parseInt(ms),
		hour_end: parseInt(he),
		min_end: parseInt(me),
		temp0: t0,
		temp100: t100,
		days: days
	});

	// =====================
	// ENVIAR AL SERVIDOR
	// =====================
	fetch("/fan/set_register.json", {
		method: "POST",
		headers: { "Content-Type": "application/json" },
		body: json
	})
		.then(async response => {

			if (response.status === 409) {
				// CONFLICTO DETECTADO → HORARIO SOLAPADO
				let msg = await response.text();

				alert(
					"⚠ Conflicto detectado:\n\n" +
					"Ya existe otro registro activo en este mismo horario " +
					"y con los mismos días seleccionados.\n" +
					"Debes modificar el horario o los días."
				);

				// Marcar visualmente el registro en conflicto
				let card = document.querySelector(`#reg_card_${id}`);
				if (card) {
					card.classList.add("error-border");
					setTimeout(() => card.classList.remove("error-border"), 2500);
				}

				throw new Error("Registro solapado");
			}

			if (!response.ok) {
				alert("❌ Error inesperado al guardar el registro, ya existe un resgistro guardado en este horario.");
				throw new Error("Error desconocido");
			}

			alert("✔ Registro actualizado correctamente.");
		})
		.catch(err => console.error("Error al guardar:", err));
}

//nueva función para enviar los datos del registro al servidor
function send_register() {

	let id = parseInt(document.getElementById("selectNumber").value);
	let hour_start = parseInt(document.getElementById("hour_start").value);
	let min_start = parseInt(document.getElementById("min_start").value);
	let hour_end = parseInt(document.getElementById("hour_end").value);
	let min_end = parseInt(document.getElementById("min_end").value);
	let temp0 = parseFloat(document.getElementById("temp0").value);
	let temp100 = parseFloat(document.getElementById("temp100").value);
	
	let active = document.getElementById("reg_active").checked ? 1 : 0;

	let daysMask = 0;
	const dayIds = ["day_mon", "day_tue", "day_wed", "day_thu", "day_fri", "day_sat", "day_sun"];
	for (let i = 0; i < 7; i++) {
		if (document.getElementById(dayIds[i]).checked)
			daysMask |= (1 << i);
	}

	// ============================================================
	// Forma en que lo hago yo (usando fetch)
	// ============================================================

	let body = JSON.stringify({
		id: id,
		active: active,
		hour_start: hour_start,
		min_start: min_start,
		hour_end: hour_end,
		min_end: min_end,
		temp0: temp0,
		temp100: temp100,
		days : daysMask
	});
	console.log("Enviando registro:", body);

	fetch("/fan/set_register.json", {
		method: "POST",
		headers: {"Content-Type": "application/json"},
		body: body
	})
	.then(response => response.json())
	.then(resp => {
		console.log("Registro guardado:", resp);
		alert("Registro actualizado correctamente.");
	})
	.catch(err => console.error("Error al guardar:", err));
	// ============================================================
	// Forma en lo que lo hace el docente (usando jQuery)
	// ============================================================
	// 	$.ajax({
	// 		url: "/fan/set_register.json",
	// 		method: "POST",
	// 		contentType: "application/json",
	// 		data: body,
	// 		success: function (resp) {
	// 			console.log("Registro guardado:", resp);
	// 			alert("Registro actualizado correctamente.");
	// 		},
	// 		error: function (xhr, status, err) {
	// 			console.error("Error al guardar:", xhr.responseText);
	// 			alert("Error al guardar el registro.");
	// 		}
	// 	});
	// }
}

// nueva función para borrar un registro
function erase_register() {
	let id = parseInt(document.getElementById("selectNumber").value);

	let emptyRegister = {
		id: id,
		active: 0,
		hour_start: 0,
		min_start: 0,
		hour_end: 0,
		min_end: 0,
		temp0: 24.0,
		temp100: 30.0,
		days: 0
	};

	fetch("/fan/set_register.json", {
		method: "POST",
		headers: { "Content-Type": "application/json" },
		body: JSON.stringify(emptyRegister)
	})
		.then(() => {
			alert("Registro borrado correctamente.");
		})
		.catch(err => console.error("Error borrando registro:", err));
}

//nueva función para obtener los valores de los registros y mostrarlos en la interfaz
function getregValues() {

	function decodeDays(mask) {
		const dayNames = ["Lun", "Mar", "Mié", "Jue", "Vie", "Sáb", "Dom"];
		let out = [];

		for (let i = 0; i < 7; i++) {
			if (mask & (1 << i)) out.push(dayNames[i]);
		}

		return out.length > 0 ? out.join(", ") : "Ninguno";
	}

	for (let i = 0; i < 3; i++) {
		fetch(`/fan/get_register.json?id=${i}`)
			.then(res => res.json())
			.then(r => {

				let text =
					(r.active ? "Activo" : "Inactivo") +
					` | ${r.hour_start.toString().padStart(2, "0")}:${r.min_start.toString().padStart(2, "0")} - ` +
					`${r.hour_end.toString().padStart(2, "0")}:${r.min_end.toString().padStart(2, "0")}` +
					` | ${r.temp0}°C → ${r.temp100}°C` +
					` | Días: ${decodeDays(r.days)}`;

				document.getElementById(`reg_${i + 1}`).innerText = text;
			});
	}
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
function updateLocalTime() {
	const now = new Date();
	const hh = now.getHours().toString().padStart(2, "0");
	const mm = now.getMinutes().toString().padStart(2, "0");
	const ss = now.getSeconds().toString().padStart(2, "0");

	const el = document.getElementById("current_time");
	if (el) el.textContent = `${hh}:${mm}:${ss}`;
}

// Actualizar cada segundo
setInterval(updateLocalTime, 1000);
updateLocalTime();












    










    


