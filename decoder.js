function get_num(x, min, max, precision, round) {
  
	var range = max - min;
	var new_range = (Math.pow(2, precision) - 1) / range;
	var back_x = x / new_range;
	
	if (back_x===0) {
		back_x = min;
	}
	else if (back_x === (max - min)) {
		back_x = max;
	}
	else {
		back_x += min;
	}
	return Math.round(back_x*Math.pow(10,round))/Math.pow(10,round);
}

function Decoder(bytes) {

    var decoded = {};
    if (port === 10){
        decoded.lat = ((bytes[0]<<16)>>>0) + ((bytes[1]<<8)>>>0) + bytes[2];
        decoded.lat = (decoded.lat / 16777215.0 * 180) - 90;
        decoded.lon = ((bytes[3]<<16)>>>0) + ((bytes[4]<<8)>>>0) + bytes[5];
        decoded.lon = (decoded.lon / 16777215.0 * 360) - 180;
      }
    return decoded;
  }