var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var json = JSON.parse(getJson());

var YELLOW = "#ffcc00";
var RED = "#ff0000";
var GREEN = "#009900";
var GRAY = "#999999";
var PURPLE = "#5c00e6"

var OFFLINE_YELLOW = "#ffE066";
var OFFLINE_RED = "#ff9999";
var OFFLINE_GREEN = "#99ff99";
var OFFLINE_GRAY = "#cccccc";
var OFFLINE_PURPLE = "#a366ff"

var DEVICE_GARAGE = "1";

connection.onopen = function () {
    connection.send("Connect " + new Date());
};
connection.onerror = function (error) {
    console.log("WebSocket Error ", error);
};
connection.onmessage = function (e) {
    console.log("Server: ", e.data);
};
connection.onclose = function(){
    console.log("WebSocket connection closed");
};

setInterval(getJson, 1000);

function sendRGB() {
    var r = document.getElementById("r").value**2/1023;
    var g = document.getElementById("g").value**2/1023;
    var b = document.getElementById("b").value**2/1023;

    var rgb = r << 20 | g << 10 | b;
    var rgbstr = "#"+ rgb.toString(16);
    console.log("RGB: " + rgbstr);
    connection.send(rgbstr);
}

function rainbowEffect(){
    rainbowEnable = ! rainbowEnable;
    if(rainbowEnable){
        connection.send("R");
        document.getElementById('rainbow').style.backgroundColor = '#00878F';
        document.getElementById('r').className = 'disabled';
        document.getElementById('g').className = 'disabled';
        document.getElementById('b').className = 'disabled';
        document.getElementById('r').disabled = true;
        document.getElementById('g').disabled = true;
        document.getElementById('b').disabled = true;
    } else {
        connection.send("N");
        document.getElementById('rainbow').style.backgroundColor = '#999';
        document.getElementById('r').className = 'enabled';
        document.getElementById('g').className = 'enabled';
        document.getElementById('b').className = 'enabled';
        document.getElementById('r').disabled = false;
        document.getElementById('g').disabled = false;
        document.getElementById('b').disabled = false;
        sendRGB();
    }
}

function buttonOne() {
	connection.send("1");
	console.log("Button One");
}
function buttonTwo() {
	connection.send("2");
	console.log('Button Two');
}
function buttonThree() {
	connection.send("3");
	console.log('Button Three');
}

function b1function(button) {

	console.log("***** B1 Function ******");
	console.log ("Button.id = " + button.id);
	connection.send("#" + button.id);
}

function saveSettings() {
	var i;
	console.log("***** saveSettings() ******");
	for (i= 0; i< json.devices.length; i++) {
		var value = document.getElementById("T"+json.devices[i].mac).value;
		var st = "*" + json.devices[i].mac + "?deviceName=" + value;
		connection.send(st);
		console.log(st);
	}
	connection.send("S");
	alert("Settings saved...");
	console.log("Setting saved.");
}

function addDoorButton (deviceName, buttonid) {
	var b1 = document.createElement("BUTTON"); // Create Button
	if (json.devices.length > 4) b1.style.width = "45%"; else b1.style.width = "95%";
	b1.style.height = "80px";
	b1.style.margin = "5px";
	b1.style.background = "blue";
	b1.style.borderRadius = "10px";
	b1.style.color = "white";
	b1.style.fontSize = "24px";
	b1.id = buttonid;

	// Assign text to your button
	b1.textContent = deviceName;

	// Register click handlers to call respective functions
	b1.onclick = function() {b1function(this);};

	// Append them in your DOM i.e to show it on your page.
	// Suppose to append it to an existing element in your page with id as "appp".
	var attachTo = document.getElementById("buttons");
	attachTo.appendChild(b1);
}

function addDoorTextBox (deviceName, textBoxID) {
	var control = document.createElement("input"); // Create Button
	if (json.devices.length > 4) control.style.width = "45%"; else control.style.width = "95%";
	control.style.height = "40px";
	control.style.margin = "5px";
//	control.style.background = "blue";
	control.style.borderRadius = "10px";
	control.style.color = "black";
	control.style.fontSize = "24px";
	control.maxLength = 15;
	control.id = textBoxID;

	// Assign text to your button
	control.value = deviceName;

	var attachTo = document.getElementById("settingsControls"); //attach to settings area on html
	attachTo.appendChild(control);
}

function loadMainControls() {
	var i;
	var jsonString = getJson();
	if (jsonString !== "null") {
		json = JSON.parse(jsonString);
		for (i= 0; i< json.devices.length; i++) {
			addDoorButton(json.devices[i].deviceName, json.devices[i].mac);
		}
	}
}

function loadSettingsControls() {
	console.log("**** loadSettingsControls ****")
	var i;
    json = JSON.parse(getJson());

	for (i= 0; i< json.devices.length; i++) {
		addDoorTextBox(json.devices[i].deviceName, "T" + json.devices[i].mac);
	}
}

setInterval(getJson, 2000);

function getJson(){
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET","/var.json",false);
    Httpreq.send(null);
    console.log("Response " + Httpreq.responseText);

    if (Httpreq.responseText !== "null") {

		json = JSON.parse(Httpreq.responseText);

		var i;
		for (i=0; i<json.devices.length; i++) {
			var sensors = (json.devices[i].online*100) + (json.devices[i].sensor0*10) + json.devices[i].sensor1;
			if (document.getElementById(json.devices[i].mac) != null) {
				switch (sensors) {
				//online values
					case 101: document.getElementById(json.devices[i].mac).style.backgroundColor = RED; break;
					case 100: document.getElementById(json.devices[i].mac).style.backgroundColor = YELLOW; break;
					case 110: document.getElementById(json.devices[i].mac).style.backgroundColor = GREEN; break;
					case 111: document.getElementById(json.devices[i].mac).style.backgroundColor = PURPLE; break;
				//offline values
					case 1: document.getElementById(json.devices[i].mac).style.backgroundColor = OFFLINE_RED; break;
					case 0: document.getElementById(json.devices[i].mac).style.backgroundColor = OFFLINE_YELLOW; break;
					case 10: document.getElementById(json.devices[i].mac).style.backgroundColor = OFFLINE_GREEN; break;
					case 11: document.getElementById(json.devices[i].mac).style.backgroundColor = OFFLINE_PURPLE; break;
				}
				document.getElementById(json.devices[i].mac).innerHTML = json.devices[i].deviceName;
			}
		}
	}

    return Httpreq.responseText;
}



