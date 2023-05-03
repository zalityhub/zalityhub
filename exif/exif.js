var ExifImage = require('exif').ExifImage;
 
try {
    new ExifImage({ image : 'c:/users/hbray/OneDrive/Pictures/DCIM/131102_153443.jpg' }, function (error, exifData) {
        if (error)
            console.log('Error: '+error.message);
        else
            console.log(exifData); // Do something with your data!
    });
} catch (error) {
    console.log('Error: ' + error.message);
}
