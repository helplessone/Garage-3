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
	console.log('**** Garage Button Pressed ******');
	console.log ('Button.id = ' + button.id);
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
	connection.send('S');
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

function addDoorTextBox (deviceName, textBoxID, container) {
	var tb = document.createElement('input'); // Create Button
	tb.style.width = '95%';
//	if (json.devices.length > 4) tb.style.width = '45%'; else tb.style.width = '95%';
	tb.style.height = '40px';
	tb.style.marginLeft = '5px';
	tb.style.marginRight = '5px';
	tb.style.borderRadius = '10px';
	tb.style.color = 'black';
	tb.style.fontSize = '24px';
	tb.maxLength = 15;
	tb.id = textBoxID;

	// Assign text to your button
	tb.value = deviceName;

//	var container = document.getElementById("settingsControls"); //attach to settings area on html
	container.appendChild(tb);
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

function moveUp(button) {
	console.log('Move Up ' + button.id);
	var st = "*" + button.id + "?moveDevice=up";
	connection.send(st);
	location.reload();
}

function moveDown(button) {
	console.log('Move Down ' + button.id);
	var st = "*" + button.id + "?moveDevice=down";
	connection.send(st);
	location.reload();
}

function deleteDoor(button) {
	var i;
	json = JSON.parse(getJson());
	console.log("json.devices.length = " + json.devices.length);
	console.log("*" + button.id + "*");
	for (i=0; i<json.devices.length; i++){
		console.log("|" + json.devices[i].mac + "|");
		console.log("");
		if (json.devices[i].mac === button.id) {
			console.log("found it");
			break;
		}
	}
	console.log("i = " + i);
	if (i < json.devices.length) {
		if (json.devices[i].online === 1) {
			window.alert("Unplug device before trying to delete it");
		} else {
			if (window.confirm("Delete device?\nAre you sure?")) {
				console.log('Delete Door ' + button.id);
				var st = "*" + button.id + "?deleteDevice=";
				connection.send(st);
				location.reload();
			}
		}
	}
}

function createSettingsTable(rows) {
	json = JSON.parse(getJson());

	//body reference
	var container = document.getElementById("settingsControls");

	// create elements <table> and a <tbody>
	var tbl = document.createElement("table");
	tbl.style.width = '100%';
	tbl.style.marginLeft = '0px';
	tbl.style.marginRight = '0px';
	var tblBody = document.createElement("tbody");

	// cells creation
	for (var j = 0; j < json.devices.length; j++) {
	// table row creation
		var row = document.createElement("tr");

		var linebreak = document.createElement("br");

		for (var i = 0; i < 3; i++) {
			// create element <td> and text node
			//Make text node the contents of <td> element
			// put <td> at end of the table row
			var cell = document.createElement("td");
			var cellText = document.createTextNode(" row " + j + ", col " + i);

			if (i === 0) {
				addDoorTextBox(json.devices[j].deviceName, "T" + json.devices[j].mac, cell);
			}
			if (i === 1) {
				cell.style.width = '50px';
				var b1 = document.createElement('img');
				b1.id = json.devices[j].mac;
				b1.src = '/trash-can.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.onclick = function() {deleteDoor(this);};
				cell.appendChild(b1);
			}
			if (i === 2) {
				cell.style.width = '50px';
				var b1 = document.createElement('img');
				b1.id = json.devices[j].mac;
				b1.src = '/arrow-up.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.onclick = function() {moveUp(this);};
				cell.appendChild(b1);

				cell.appendChild(linebreak);

				var b2 = document.createElement('img');
				b2.id = json.devices[j].mac;
				b2.src = '/arrow-up.png';
				b2.style.width = '30px';
				b2.style.height = '30px';
				b2.style.transform = 'rotate(180deg)';
				b2.onclick = function() {moveDown(this);};
				cell.appendChild(b2);
			}

			row.appendChild(cell);
		}

		//row added to end of table body
		tblBody.appendChild(row);
	}

	// append the <tbody> inside the <table>
	tbl.appendChild(tblBody);
	// put <table> in the <settingsControls>
	container.appendChild(tbl);
	// tbl border attribute to
	tbl.setAttribute("border", "0");
}

function loadSettingsControls() {
	console.log("**** loadSettingsControls ****")
/*
	var i;
  	var fieldset = document.createElement ("fieldset");

	var legend = document.createElement ("legend");
	legend.innerHTML = "Garage Door Settings";
	fieldset.appendChild (legend);

	var autoClose = document.createElement("input");
	autoClose.setAttribute ("type", "checkbox");
	fieldset.appendChild (autoClose);

	var attachTo = document.getElementById("settingsControls"); //attach to settings area on html
	attachTo.appendChild (fieldset);

    json = JSON.parse(getJson());
*/

    createSettingsTable(json.devices.length);
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
			if (json.devices[i].online !== 'null') {
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
	}

    return Httpreq.responseText;
}



