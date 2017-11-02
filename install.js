const os = require('os');
var spawn = require('cross-spawn');

if (os.platform() === 'sunos' || os.platform() === 'freebsd') {
    spawn.sync('npm', ['run', 'native_build'], {
        input: 'solaris detected. Build native module.',
        stdio: 'inherit'
    });
}else{
console.log("OS Not supported");
}
