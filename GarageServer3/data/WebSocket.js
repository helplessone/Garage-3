var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var json = JSON.parse(getJson());

var YELLOW = "#ffd11a";
var RED = "#ff0000";
var GREEN = "#009900";
var GRAY = "#999999";
var PURPLE = "#5c00e6"
var WHITE = "#ffffff"

var OFFLINE_YELLOW = "#ffeb99";
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

function addDoorButton (deviceName, mac) {
	var b1 = document.createElement("BUTTON"); // Create Button
	if (json.devices.length > 4) b1.style.width = "45%"; else b1.style.width = "95%";
	b1.style.height = "80px";
	b1.style.margin = "5px";
	b1.style.background = "blue";
	b1.style.borderRadius = "10px";
	b1.style.color = "white";
	b1.style.fontSize = "30px";
	b1.id = mac;
	b1.setAttribute('mac',mac);

	// Assign text to your button
//	b1.textContent = deviceName + "</br>" + mac;
//	b1.innerHTML = deviceName;

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

function buttonPress(image){
	console.log ('Button press...' + image.getAttribute('mac'));
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
	var st = "*" + button.getAttribute("mac") + "?moveDevice=up";
	connection.send(st);
	location.reload();
}

function moveDown(button) {
	console.log('Move Down ' + button.id);
	var st = "*" + button.getAttribute("mac") + "?moveDevice=down";
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
		if (json.devices[i].mac === button.getAttribute("mac")) {
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
				var st = "*" + button.getAttribute("mac") + "?deleteDevice=";
				connection.send(st);
				location.reload();
			}
		}
	}
}

function swapSensors(button){
	console.log("swapSensors " + button.id);
	json = JSON.parse(getJson());
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].sensor0 !== json.devices[i].sensor1) { // you can only swap when open or closed
			console.log("sensorSwap = " + json.devices[i].sensorSwap);
			if (json.devices[i].sensorSwap === 0) value = 'TRUE'; else value = 'FALSE';
			var st = "*" + json.devices[i].mac + "?sensorSwap=" + value;

			connection.send(st);
			console.log(st);
			connection.send('S');
			console.log("Setting saved.");
			location.reload();
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

		for (var i = 0; i < 4; i++) {
			// create element <td> and text node
			//Make text node the contents of <td> element
			// put <td> at end of the table row
			var cell = document.createElement("td");
			var cellText = document.createTextNode(" row " + j + ", col " + i);

			if (i === 0) {
				addDoorTextBox(json.devices[j].deviceName, "T" + json.devices[j].mac, cell);
			}
			if (i === 1) { //swap button
				cell.style.width = '50px';
				var b1 = document.createElement('img');
				b1.id = 'swap' + json.devices[j].mac;
				b1.setAttribute("mac",json.devices[j].mac);
				b1.src = '/reverse.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.style.backgroundColor = getDeviceColor(json.devices[j]);
				b1.onclick = function() {swapSensors(this);};
				if (json.devices[j].sensor0 === json.devices[j].sensor1) { //if door is open or closed alow sensor swap
					b1.style.visibility = 'hidden';
				}
				cell.appendChild(b1);
			}
			if (i === 2) {
				cell.style.width = '50px';
				if (json.devices[j].online === 0) { // if door if offline, allow delete
					var b1 = document.createElement('img');
//					b1.id = json.devices[j].mac;
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src = '/trash-can.png';
					b1.style.width = '30px';
					b1.style.height = '30px';
//					b1.style.backgroundColor = WHITE;
					b1.onclick = function() {deleteDoor(this);};
					cell.appendChild(b1);
				}
			}
			if (i === 3) {
				cell.style.width = '50px';
				var b1 = document.createElement('img');
//				b1.id = json.devices[j].mac;
				b1.setAttribute("mac",json.devices[j].mac);
				b1.src = '/arrow-up.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.onclick = function() {moveUp(this);};
				cell.appendChild(b1);

				cell.appendChild(linebreak);

				var b2 = document.createElement('img');
//				b2.id = json.devices[j].mac;
				b2.setAttribute("mac",json.devices[j].mac);
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

	var b1 = document.createElement('img');
	b1.src = '/reverse.png';
	b1.style.width = '30px';
	b1.style.height = '30px';
	b1.style.vericalAlign = 'bottom';

	var s = document.createElement('span');
	s.style.fontSize = '18px';
	var txt = document.createTextNode(" = Switch Door Position");
	s.appendChild(b1);
	s.appendChild(txt);
	container.appendChild(s);
}

function loadSettingsControls() {
	console.log("**** loadSettingsControls ****")

    createSettingsTable(json.devices.length);
}

setInterval(getJson, 2000);

function getDeviceColor(device){
	var sensors = (device.online*100) + (device.sensor0*10) + device.sensor1;
	if (device.sensorSwap === 0) {
		switch (sensors) {
			//online values
			case 101: return RED; break;
			case 100: return YELLOW; break;
			case 110: return GREEN; break;
			case 111: return PURPLE; break;
			//offline values
			case 1: return OFFLINE_RED; break;
			case 0: return OFFLINE_YELLOW; break;
			case 10: return OFFLINE_GREEN; break;
			case 11: return OFFLINE_PURPLE; break;
		}
	} else {
		switch (sensors) {
			//online values
			case 110: return RED; break;
			case 100: return YELLOW; break;
			case 101: return GREEN; break;
			case 111: return PURPLE; break;
			//offline values
			case 10: return OFFLINE_RED; break;
			case 0: return OFFLINE_YELLOW; break;
			case 1: return OFFLINE_GREEN; break;
			case 11: return OFFLINE_PURPLE; break;
		}
	}
}

function getJson(){
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET","/var.json",false);
    Httpreq.send(null);
    console.log("Response " + Httpreq.responseText);

    if (Httpreq.responseText !== "null") {
		json = JSON.parse(Httpreq.responseText);

		var i;
		for (i=0; i<json.devices.length; i++) {
			var button = document.getElementById(json.devices[i].mac);
			if (button != null) {
				button.style.backgroundColor = getDeviceColor(json.devices[i]);
				var st;
				var color = getDeviceColor(json.devices[i]);
				switch (color) {
					case RED: st = "Open since "; break;
					case GREEN: st = "Closed since "; break;
					case YELLOW: st = "Partially Open since "; break;
					case PURPLE: st = "Invalid State since "; break;
					default: st = "Offline since ";
				}
				var date = parseInt(json.devices[i].deviceTime);
				var year = date >> 20 & 0x1f;
				var month = date >> 16 & 0xf;
				var day = date >> 11 & 0x1f;
				var hour = date >> 6 & 0x1f;
				var am = 'a';
				if (hour === 12) am = 'p';
				if (hour > 12) {
					hour = hour - 12;
					am = 'p';
				}
				if (hour === 0) {
					hour = 12;
					am = 'a';
				}
				var minute = date & 0x1f;
				var min = minute.toString();
				while (min.length < 2) min = '0' + min;

				button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + " " + month + "/" + day + " " + hour + ":" + min + am + "</p>";
			}
			var swapbutton = document.getElementById('swap' + json.devices[i].mac);
			if (swapbutton != null) {
				if (json.devices[i].sensor0 === json.devices[i].sensor1) { //if door is open or closed alow sensor swap
					swapbutton.style.visibility = 'hidden';
				} else swapbutton.style.visibility = 'visible';
				swapbutton.style.backgroundColor = getDeviceColor(json.devices[i]);
			}
		}
	}
    return Httpreq.responseText;
}

/* When the user clicks on the button,
toggle between hiding and showing the dropdown content */
function myFunction() {
  document.getElementById("myDropdown").classList.toggle("show");
}

// Close the dropdown menu if the user clicks outside of it
window.onclick = function(event) {
  if (!event.target.matches('.dropbtn')) {
    var dropdowns = document.getElementsByClassName("dropdown-content");
    var i;
    for (i = 0; i < dropdowns.length; i++) {
      var openDropdown = dropdowns[i];
      if (openDropdown.classList.contains('show')) {
        openDropdown.classList.remove('show');
      }
    }
  }
}

