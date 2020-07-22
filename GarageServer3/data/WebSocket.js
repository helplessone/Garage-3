var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var json = getJson();

var YELLOW = "#ffd11a";
var RED = "#ff0000";
var GREEN = "#009900";
var GRAY = "#999999";
var PURPLE = "#5c00e6"
var WHITE = "#ffffff"
var BLUE = "#0000ff"

var OFFLINE_YELLOW = "#ffeb99";
var OFFLINE_RED = "#ff9999";
var OFFLINE_GREEN = "#99ff99";
var OFFLINE_GRAY = "#cccccc";
var OFFLINE_PURPLE = "#a366ff"
var OFFLINE_BLUE = "#8080ff"

var DEVICE_GARAGE = '1';
var DEVICE_THERMOMETER = '2';

/********* GLOBAL VARIBLES ************/
var timeOffset = 0;
/**************************************/

connection.onopen = function () {
	connection.send("Connect " + new Date());
	console.log("connection.onopen = " + Date.now().toString());
};
connection.onerror = function (error) {
    console.log("WebSocket Error ", error);
};
connection.onmessage = function (e) {
	processJson(e.data);
};
connection.onclose = function(){
    console.log("WebSocket connection closed");
};

// setInterval(getJson, 200);

function mainButtonPressed(button) {
	console.log('**** Button Pressed ******');
	console.log ('Button.id = ' + button.id + ' mac = ' + button.getAttribute('mac'));
	connection.send("#" + button.id);
}

function postSettings(json) {
	var url = "http://192.168.7.83/var.json";
	var params = json;
	var postRequest = new XMLHttpRequest();
	postRequest.open("POST", url, true);

	//Send the proper header information along with the request
	postRequest.setRequestHeader("Content-type", "application/x-www-form-urlencoded"); 
	postRequest.send(json);
}

function getDeviceJson(device) {
	devicejson = {};

	devicejson['mac'] = device.mac;
	devicejson['deviceName'] = document.getElementById("DEVICENAME"+device.mac).value;
	if (device.deviceType == DEVICE_GARAGE) {
		devicejson['closeDelay'] = parseInt(document.getElementById("CLOSEDELAY"+device.mac).value); 
		
		var value = document.getElementById("CLOSETIME"+device.mac).value

		value = value.replace(/\D/g, '');
		console.log("closeTime = " + parseInt(value));

		devicejson['closeTime'] = parseInt(value); 
	}
	if (device.deviceType == DEVICE_THERMOMETER) {
		devicejson['minTemp'] = parseInt(document.getElementById("MINTEMP"+device.mac).value); 
		devicejson['maxTemp'] = parseInt(document.getElementById("MAXTEMP"+device.mac).value); 
		devicejson['celcius'] = document.getElementById("CELCIUS"+device.mac).checked?1:0;
	}
	return devicejson;
}

function getGlobalsJson() {
	globalsjson = {};
	globalsjson['timeOffset'] = timeOffset;
	return globalsjson;
}

function saveGlobals() {
	connection.send('^' + JSON.stringify(getGlobalsJson()));	
}

function saveSettings() {
	var i;
	console.log("***** saveSettings() ******");
	for (i= 0; i< json.devices.length; i++) {
		var st = '^' + JSON.stringify(getDeviceJson(json.devices[i]));
		console.log(st);	
		connection.send(st);
	}
	saveGlobals();
	console.log("Setting saved.");
}

function deleteSettings() {
	connection.send('D');
	console.log("Setting deleted");	
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
	b1.setAttribute('mac', mac);

	// Register click handlers to call respective functions
	b1.onclick = function() {mainButtonPressed(this);};

	// Append them in your DOM i.e to show it on your page.
	// Suppose to append it to an existing element in your page with id as "appp".
	var attachTo = document.getElementById("buttons");
	attachTo.appendChild(b1);
}

function addDoorTextBox (device, container) {
	var br = document.createElement("br");
	console.log(device);

	if (device.deviceType == DEVICE_GARAGE) {
		var closeDelay = document.createElement('input'); // closeDelay to automatically close
		closeDelay.style.width = '15%';
		closeDelay.style.height = '20px';
		closeDelay.style.marginTop = '4px';
		closeDelay.style.marginLeft = '5px';
		closeDelay.style.marginRight = '5px';
		closeDelay.style.textAlign = 'center';
		closeDelay.style.borderRadius = '10px';
		closeDelay.style.color = 'black';
		closeDelay.style.fontSize = '20px';
		closeDelay.maxLength = 3;
		closeDelay.id = 'CLOSEDELAY' + device.mac;
		closeDelay.setAttribute('mac',device.mac);
		closeDelay.value = device.closeDelay;

		var closeTime = document.createElement('input'); // time to automatically close
		closeTime.style.width = '20%';
		closeTime.style.height = '20px';
		closeTime.style.marginTop = '4px';
		closeTime.style.marginLeft = '5px';
		closeTime.style.marginRight = '5px';
		closeTime.style.marginBottom = '3px';
		closeTime.style.textAlign = 'center';
		closeTime.style.borderRadius = '10px';
		closeTime.style.color = 'black';
		closeTime.style.fontSize = '20px';
		closeTime.maxLength = 5;
		closeTime.id = 'CLOSETIME' + device.mac;
		closeTime.setAttribute('mac',device.mac);
		closeTime.value = device.closeTime;
		
		var t1 = document.createTextNode("AUTO CLOSE AFTER");
		var t2 = document.createTextNode("MINUTES");
		var t3 = document.createTextNode("AUTO CLOSE AT");
		var t4 = document.createTextNode("(9pm=2100)");
	}	

	
	if (device.deviceType == DEVICE_THERMOMETER) {
		var min = document.createElement('input'); // min input		
		min.style.width = '15%';
		min.style.height = '20px';
		min.style.marginTop = '4px';
		min.style.marginLeft = '0px';
		min.style.marginRight = '10px';
		min.style.marginBottom = '3px';
		min.style.textAlign = 'center';
		min.style.borderRadius = '10px';
		min.style.color = 'black';
		min.style.fontSize = '20px';
		min.maxLength = 3;
		min.id = 'MINTEMP' + device.mac;
		min.value = device.minTemp;

		var max = document.createElement('input'); // max input
		max.style.width = '15%';
		max.style.height = '20px';
		max.style.marginTop = '4px';
		max.style.marginLeft = '2px';
		max.style.marginRight = '10px';
		min.style.marginBottom = '3px';
		max.style.textAlign = 'center';
		max.style.borderRadius = '10px';
		max.style.color = 'black';
		max.style.fontSize = '20px';
		max.maxLength = 3;
		max.id = 'MAXTEMP' + device.mac;
		max.value = device.maxTemp;
		
		var celcius = document.createElement('input'); // celcius check box
		celcius.setAttribute("type", "checkbox");
		celcius.style.width = '10%';
		celcius.style.height = '20px';
		celcius.style.marginBottom = '6px';
		celcius.style.marginLeft = '2px';
		celcius.style.marginRight = '0px';
		celcius.style.verticalAlign = 'bottom';
		celcius.style.color = 'black';
		celcius.style.fontSize = '20px';
		celcius.id = 'CELCIUS' + device.mac;
		celcius.checked = (device.celcius == 1)?true:false;
		
		var mn = document.createTextNode("MIN ");
		var mx = document.createTextNode("MAX ");
		var cel = document.createTextNode("\xB0C");
	}

	var tb = document.createElement('input'); // Create Button
	if (device.deviceType == DEVICE_GARAGE) tb.style.width = '95%';
	else if (device.deviceType == DEVICE_THERMOMETER) tb.style.width = '95%';

	tb.style.height = '40px';
	tb.style.marginTop = '2px';
	tb.style.marginLeft = '0px';
	tb.style.marginRight = '0px';
	tb.style.borderRadius = '10px';
	tb.style.color = 'black';
	tb.style.fontSize = '24px';
	tb.maxLength = 15;
	tb.id = 'DEVICENAME' + device.mac;
	tb.value = device.deviceName;

	container.appendChild(tb);

	if (device.deviceType == DEVICE_GARAGE) {
		container.appendChild(t1);		
		container.appendChild(closeDelay);
		container.appendChild(t2);
		container.appendChild(br);
		container.appendChild(t3);
		container.appendChild(closeTime);
		container.appendChild(t4);

	}
	if (device.deviceType == DEVICE_THERMOMETER) {
		container.appendChild(mn);		
		container.appendChild(min);
		container.appendChild(mx);
		container.appendChild(max);
		container.appendChild(cel);
		container.appendChild(celcius);
	}

}

function buttonPress(image){
	console.log ('Button press...' + image.getAttribute('mac'));
}

function loadMainControls() {
	var i;
	var json = getJson();
	if (json !== "null") {
		for (i= 0; i< json.devices.length; i++) {
			addDoorButton(json.devices[i].deviceName, json.devices[i].mac);
		}
	}
	json = getJson();	//reload json to get button text
}

function timeString(time) {

	var currDate = new Date(time * 1000);
	currDate.setHours(currDate.getHours() + timeOffset);

	var month = (currDate.getUTCMonth()+1).toString();
	var day = currDate.getUTCDate().toString();
	var hour = currDate.getUTCHours().toString();
	var minutes = currDate.getUTCMinutes().toString();
	while (minutes.length < 2) minutes = '0' + minutes;
	while (hour.length < 2) hour = '0' + hour;

	var formattedTime = month + '/' + day + ' ' + hour + ':' + minutes;

	return formattedTime;
}

function displayTime(){
	var b = document.getElementById('timeButton');
	json = getJson();
	b.innerHTML = timeString(json.global.currTime);
}

function incTime(){
	timeOffset++;
	saveGlobals();
	displayTime();

}

function decTime(){
	timeOffset--;
	saveGlobals();
	displayTime();
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
	json = getJson();
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
	json = getJson();
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].sensor0 !== json.devices[i].sensor1) { // you can only swap when open or closed
			console.log("sensorSwap = " + json.devices[i].sensorSwap);
			if (json.devices[i].sensorSwap === 0) value = 1; else value = 0;

			devicejson = {};
			devicejson['mac'] = json.devices[i].mac;
			devicejson['sensorSwap'] = value;	
			var st = JSON.stringify(devicejson);
			console.log("sensorSwap json = " + st);
			connection.send('^' + st);
		}
	}
}

function createSettingsTable(rows) {
	json = getJson();
	var br = document.createElement("br");

	//body reference
	var container = document.getElementById("settingsControls");

	// cells creation
	for (var j = 0; j < json.devices.length; j++) {
		var tbl = document.createElement("table"); //create new table for each device
		tbl.style.width = '100%';
		tbl.style.marginLeft = '0px';
		tbl.style.marginRight = '0px';
		tbl.style.border = "thin solid black";
		var tblBody = document.createElement("tbody");

	// table row creation
		var row = document.createElement("tr");
		row.className = 'settingsTableRow';

		for (var i = 0; i < 3; i++) {
			// create element <td> and text node
			//Make text node the contents of <td> element
			// put <td> at end of the table row
			var cell = document.createElement("td");
//			cell.className = 'settingsTableCell';

			if (i === 0) {
				addDoorTextBox(json.devices[j], cell);
			}
			if (i === 1) { //swap sensor button (garage only)
				if (json.devices[j].deviceType == DEVICE_GARAGE) {
					cell.style.width = '50px';
					var b1 = document.createElement('img');
					b1.id = 'swap' + json.devices[j].mac;
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src = '/reverse.png';
					b1.style.width = '30px';
					b1.style.height = '30px';
					b1.style.backgroundColor = getDeviceColor(json.devices[j]);
					b1.onclick = function() {swapSensors(this);};
					if (json.devices[j].sensor0 === json.devices[j].sensor1) { //if door is open or closed allow sensor swap
						b1.style.visibility = 'hidden';
					}
					cell.appendChild(b1);
				}

				if (json.devices[j].online === 0) { // if door if offline, allow delete
					var b1 = document.createElement('img');
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src = '/trash-can.png';
					b1.style.width = '30px';
					b1.style.height = '30px';
					b1.onclick = function() {deleteDoor(this);};
					cell.appendChild(br);
					cell.appendChild(b1);
				}				
			}
			
			if (i === 2) { //move buttons
				cell.style.width = '50px';
				var b1 = document.createElement('img');
				b1.setAttribute("mac",json.devices[j].mac);
				b1.src = '/arrow-up.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.onclick = function() {moveUp(this);};
				cell.appendChild(b1);

				cell.appendChild(br);

				var b2 = document.createElement('img');
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
		// append the <tbody> inside the <table>
		tbl.appendChild(tblBody);
		// put <table> in the <settingsControls>
		container.appendChild(tbl);	
	}




	var b1 = document.createElement('img');
	b1.src = '/reverse.png';
	b1.style.width = '30px';
	b1.style.height = '30px';
	b1.style.vericalAlign = 'bottom';

/*
	var s = document.createElement('span');
	s.style.fontSize = '18px';
	var txt = document.createTextNode(" = Switch Door Position");
	s.appendChild(b1);
	s.appendChild(txt);
	container.appendChild(s);
*/	
}

function loadSettingsControls() {
	console.log("**** loadSettingsControls ****")

	createSettingsTable(json.devices.length);
	displayTime();
}

function getDeviceColor(device){
	var sensors = (device.online*100) + (device.sensor0*10) + device.sensor1;
	if (device.sensorSwap === 0) {
		switch (sensors) {
			//online values
			case 101: return RED; break;
			case 111: return YELLOW; break;
			case 110: return GREEN; break;
			case 100: return PURPLE; break;
			//offline values
			case 10: return OFFLINE_RED; break;
			case 1: return OFFLINE_YELLOW; break;
			case 1: return OFFLINE_GREEN; break;
			case 0: return OFFLINE_PURPLE; break;
		}
	} else {
		switch (sensors) {
			//online values
			case 110: return RED; break;
			case 111: return YELLOW; break;
			case 101: return GREEN; break;
			case 100: return PURPLE; break;
			//offline values
			case 10: return OFFLINE_RED; break;
			case 11: return OFFLINE_YELLOW; break;
			case 1: return OFFLINE_GREEN; break;
			case 0: return OFFLINE_PURPLE; break;
		}
	}
}

function getJson(){
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET","/var.json",false);
    Httpreq.send(null);

	if (Httpreq.responseText !== "null") {
		processJson(Httpreq.responseText);
		json = JSON.parse(Httpreq.responseText)
		return json;
	}
	return;
}

function processJson(jsonString){

    if (jsonString !== "null") {
		json = JSON.parse(jsonString);

		timeOffset = json.global.timeOffset;
		var doc = document.getElementById('timeButton');
		if (doc !== null) doc.innerHTML = timeString(json.global.currTime);

		var i;
		for (i=0; i<json.devices.length; i++) {
			var button = document.getElementById(json.devices[i].mac);
			if (button != null) {
				if (json.devices[i].deviceType == DEVICE_GARAGE) {
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
					var deviceTime = timeString(json.devices[i].deviceTime);

					button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + " " + deviceTime + "</p>";
				}
				if (json.devices[i].deviceType == DEVICE_THERMOMETER) {
					var t;

					if (json.devices[i].celcius == true) t = Math.round(json.devices[i].temp / 100); else t = (Math.round((json.devices[i].temp / 100) * 9/5) + 32);
					
					if ((t > json.devices[i].maxTemp) && (json.devices[i].maxTemp != 0)) button.style.backgroundColor = RED;
					else if ((t < json.devices[i].minTemp) && (json.devices[i].minTemp != 0)) button.style.backgroundColor = BLUE;
					else if (json.devices[i].online == 0) button.style.backgroundColor = YELLOW;
					else button.style.backgroundColor = GREEN;


					var st = "Last Read ";
					var deviceTime = timeString(json.devices[i].deviceTime);
					var degC = Math.round(json.devices[i].temp / 100) + "\xB0 C";
					var degF = Math.round( ((json.devices[i].temp / 100) * 9/5) + 32) + "\xB0 F";
					if (json.devices[i].online == 1) button.innerHTML = "<p class='pbutton1'>" + degC + ' / ' + degF + "</p></br><p class='pbutton2'>" + json.devices[i].deviceName + " @ " + deviceTime + "</p>";
					else button.innerHTML = "<p class='pbutton1'>" + "OFFLINE" + "</p></br><p class='pbutton2'>" + json.devices[i].deviceName + " @ " + deviceTime + "</p>";
				}
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

