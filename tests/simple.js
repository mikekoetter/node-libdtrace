     var libdtrace = require('../index');
     try{
      var dtp = new libdtrace.Consumer();
        
      var prog = 'BEGIN { trace("hello world"); }';
        
      dtp.strcompile(prog);
      dtp.go();
        
      dtp.consume(function (probe, rec) {
              if (rec)
                      console.log(rec.data);
      });
  }catch(err){
  	console.log(err.message)
  }
