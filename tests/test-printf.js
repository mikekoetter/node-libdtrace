var libdtrace = require('../');
var assert = require('assert');

dtp = new libdtrace.Consumer();
dtp.strcompile('BEGIN { printf("{ foo: %d", 123); printf(", bar: %d", 456); }'); 

dtp.go();

dtp.consume(function testbasic (probe, rec) {
	assert.equal(probe.provider, 'dtrace');
	assert.equal(probe.module, '');
	assert.equal(probe.function, '');
	assert.equal(probe.name, 'BEGIN');

	console.log(probe);
	console.log(rec);
});

dtp.stop();

