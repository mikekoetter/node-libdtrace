/*
 * For whatever reason, g++ on Solaris defines _XOPEN_SOURCE -- which in
 * turn will prevent us from pulling in our desired definition for boolean_t.
 * We don't need it, so explicitly undefine it.
 */
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#include <nan.h>
#include <iostream>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <vector>

/*
 * Sadly, libelf refuses to compile if _FILE_OFFSET_BITS has been manually
 * jacked to 64 on a 32-bit compile.  In this case, we just manually set it
 * back to 32.
 */
#if defined(_ILP32) && (_FILE_OFFSET_BITS != 32)
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 32
#endif

#include <dtrace.h>

/*
 * This is a tad unsightly:  if we didn't find the definition of the
 * llquantize() aggregating action, we're going to redefine it here (along
 * with its support cast of macros).  This allows node-libdtrace to operate
 * on a machine that has llquantize(), even if it was compiled on a machine
 * without the support.
 */
#ifndef DTRACEAGG_LLQUANTIZE

#define	DTRACEAGG_LLQUANTIZE			(DTRACEACT_AGGREGATION + 9)

#define	DTRACE_LLQUANTIZE_FACTORSHIFT		48
#define	DTRACE_LLQUANTIZE_FACTORMASK		((uint64_t)UINT16_MAX << 48)
#define	DTRACE_LLQUANTIZE_LOWSHIFT		32
#define	DTRACE_LLQUANTIZE_LOWMASK		((uint64_t)UINT16_MAX << 32)
#define	DTRACE_LLQUANTIZE_HIGHSHIFT		16
#define	DTRACE_LLQUANTIZE_HIGHMASK		((uint64_t)UINT16_MAX << 16)
#define	DTRACE_LLQUANTIZE_NSTEPSHIFT		0
#define	DTRACE_LLQUANTIZE_NSTEPMASK		UINT16_MAX

#define DTRACE_LLQUANTIZE_FACTOR(x)             \
	(uint16_t)(((x) & DTRACE_LLQUANTIZE_FACTORMASK) >> \
	DTRACE_LLQUANTIZE_FACTORSHIFT)

#define DTRACE_LLQUANTIZE_LOW(x)                \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_LOWMASK) >> \
        DTRACE_LLQUANTIZE_LOWSHIFT)

#define DTRACE_LLQUANTIZE_HIGH(x)               \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_HIGHMASK) >> \
        DTRACE_LLQUANTIZE_HIGHSHIFT)

#define DTRACE_LLQUANTIZE_NSTEP(x)              \
        (uint16_t)(((x) & DTRACE_LLQUANTIZE_NSTEPMASK) >> \
        DTRACE_LLQUANTIZE_NSTEPSHIFT)
#endif


using namespace Nan;  
using namespace v8;
using std::string;
using std::vector;



class DTraceConsumer : public Nan::ObjectWrap {
public:

	static NAN_MODULE_INIT(Init)
	{



    	v8::Local<v8::FunctionTemplate> dtc = Nan::New<v8::FunctionTemplate>(New);

		dtc->SetClassName(Nan::New("Consumer").ToLocalChecked());
	    dtc->InstanceTemplate()->SetInternalFieldCount(1);


	    Nan::SetPrototypeMethod(dtc, "strcompile", DTraceConsumer::Strcompile);
	    Nan::SetPrototypeMethod(dtc, "version", DTraceConsumer::Version);
	    Nan::SetPrototypeMethod(dtc, "setopt", DTraceConsumer::Setopt);


	    Nan::SetPrototypeMethod(dtc, "go", DTraceConsumer::Go);
	    Nan::SetPrototypeMethod(dtc, "consume", DTraceConsumer::Consume);
	    Nan::SetPrototypeMethod(dtc, "aggwalk", DTraceConsumer::Aggwalk);		
	    Nan::SetPrototypeMethod(dtc, "aggclear", DTraceConsumer::Aggclear);		
	    Nan::SetPrototypeMethod(dtc, "aggmin", DTraceConsumer::Aggmin);		
	    Nan::SetPrototypeMethod(dtc, "aggmax", DTraceConsumer::Aggmax);		
	    Nan::SetPrototypeMethod(dtc, "stop", DTraceConsumer::Stop);

    	dtc_templ.Reset(dtc);

    	Nan::Set(target, Nan::New("Consumer").ToLocalChecked(),
    		Nan::GetFunction(dtc).ToLocalChecked());
	}


protected:
	DTraceConsumer();
	~DTraceConsumer();

	Handle<Value> error(const char *fmt, ...);
	Handle<Value> badarg(const char *msg);
	boolean_t valid(const dtrace_recdesc_t *);
	const char *action(const dtrace_recdesc_t *, char *, int);
	v8::Local<v8::Value>   record(const dtrace_recdesc_t *, caddr_t);
	v8::Local<v8::Object>  probedesc(const dtrace_probedesc_t *);

	v8::Local<v8::Array> *ranges_cached(dtrace_aggvarid_t);
	v8::Local<v8::Array> *ranges_cache(dtrace_aggvarid_t, Local<Array> *);
	v8::Local<v8::Array> *ranges_quantize(dtrace_aggvarid_t);
	v8::Local<v8::Array> *ranges_lquantize(dtrace_aggvarid_t, uint64_t);
	v8::Local<v8::Array> *ranges_llquantize(dtrace_aggvarid_t, uint64_t, int);

	static int consume(const dtrace_probedata_t *data,
	    const dtrace_recdesc_t *rec, void *arg);
	static int aggwalk(const dtrace_aggdata_t *agg, void *arg);
	static int bufhandler(const dtrace_bufdata_t *bufdata, void *arg);

	static NAN_METHOD(New);
	static NAN_METHOD(Consume);
	static NAN_METHOD(Aggclear);
	static NAN_METHOD(Aggwalk);
	static NAN_METHOD(Aggmin);
	static NAN_METHOD(Aggmax);
	static NAN_METHOD(Strcompile);
	static NAN_METHOD(Setopt);
	static NAN_METHOD(Go);
	static NAN_METHOD(Stop);
	static NAN_METHOD(Version);

private:
	dtrace_hdl_t *dtc_handle;

	static Nan::Persistent<FunctionTemplate> dtc_templ;
	const Nan::FunctionCallbackInfo<v8::Value> *dtc_args;
	Local<Function> dtc_callback;
	Handle<Value> dtc_error;
	Local<Array> *dtc_ranges;
	dtrace_aggvarid_t dtc_ranges_varid;


  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

};


Nan::Persistent<FunctionTemplate> DTraceConsumer::dtc_templ;

DTraceConsumer::DTraceConsumer() : Nan::ObjectWrap()
{
	int err;
	dtrace_hdl_t *dtp;

	if ((dtc_handle = dtp = dtrace_open(DTRACE_VERSION, 0, &err)) == NULL){
		Nan::ThrowError(Nan::New<v8::String>(dtrace_errmsg(NULL, err)).ToLocalChecked());
	}else{
		/*
		 * Set our buffer size and aggregation buffer size to the de facto
		 * standard of 4M.
		 */
		(void) dtrace_setopt(dtp, "bufsize", "4m");
		(void) dtrace_setopt(dtp, "aggsize", "4m");

		if (dtrace_handle_buffered(dtp, DTraceConsumer::bufhandler, this) == -1)
			throw (dtrace_errmsg(dtp, dtrace_errno(dtp)));

		dtc_ranges = NULL;
	}
};

DTraceConsumer::~DTraceConsumer()
{
	if (dtc_ranges != NULL)
		delete [] dtc_ranges;

	dtrace_close(dtc_handle);
}


NAN_METHOD (DTraceConsumer::New) {
	DTraceConsumer *dtc;
	try {
		dtc = new DTraceConsumer();
    	dtc->Wrap(info.This());
	  	info.GetReturnValue().Set(info.This());

	} catch (char const *msg) {
		Nan::ThrowError(Nan::New<v8::String>(msg).ToLocalChecked());
	}
}



const char *
DTraceConsumer::action(const dtrace_recdesc_t *rec, char *buf, int size)
{
	static struct {
		dtrace_actkind_t action;
		const char *name;
	} act[] = {
		{ DTRACEACT_NONE,	"<none>" },
		{ DTRACEACT_DIFEXPR,	"<DIF expression>" },
		{ DTRACEACT_EXIT,	"exit()" },
		{ DTRACEACT_PRINTF,	"printf()" },
		{ DTRACEACT_PRINTA,	"printa()" },
		{ DTRACEACT_LIBACT,	"<library action>" },
		{ DTRACEACT_USTACK,	"ustack()" },
		{ DTRACEACT_JSTACK,	"jstack()" },
		{ DTRACEACT_USYM,	"usym()" },
		{ DTRACEACT_UMOD,	"umod()" },
		{ DTRACEACT_UADDR,	"uaddr()" },
		{ DTRACEACT_STOP,	"stop()" },
		{ DTRACEACT_RAISE,	"raise()" },
		{ DTRACEACT_SYSTEM,	"system()" },
		{ DTRACEACT_FREOPEN,	"freopen()" },
		{ DTRACEACT_STACK,	"stack()" },
		{ DTRACEACT_SYM,	"sym()" },
		{ DTRACEACT_MOD,	"mod()" },
		{ DTRACEAGG_COUNT,	"count()" },
		{ DTRACEAGG_MIN,	"min()" },
		{ DTRACEAGG_MAX,	"max()" },
		{ DTRACEAGG_AVG,	"avg()" },
		{ DTRACEAGG_SUM,	"sum()" },
		{ DTRACEAGG_STDDEV,	"stddev()" },
		{ DTRACEAGG_QUANTIZE,	"quantize()" },
		{ DTRACEAGG_LQUANTIZE,	"lquantize()" },
		{ DTRACEAGG_LLQUANTIZE,	"llquantize()" },
		{ DTRACEACT_NONE,	NULL },
	};

	dtrace_actkind_t action = rec->dtrd_action;
	int i;

	for (i = 0; act[i].name != NULL; i++) {
		if (act[i].action == action)
			return (act[i].name);
	}

	(void) snprintf(buf, size, "<unknown action 0x%x>", action);

	return (buf);
}

Handle<Value>
DTraceConsumer::error(const char *fmt, ...)
{
	char buf[1024], buf2[1024];
	char *err = buf;
	va_list ap;

	va_start(ap, fmt);
	(void) vsnprintf(buf, sizeof (buf), fmt, ap);

	if (buf[strlen(buf) - 1] != '\n') {
		/*
		 * If our error doesn't end in a new-line, we'll append the
		 * strerror of errno.
		 */
		(void) snprintf(err = buf2, sizeof (buf2),
		    "%s: %s", buf, strerror(errno));
	} else {
		buf[strlen(buf) - 1] = '\0';
	}
	
	Nan::ThrowError(Nan::New<v8::String>(err).ToLocalChecked());
}

Handle<Value>
DTraceConsumer::badarg(const char *msg)
{
	Nan::ThrowTypeError(Nan::New<v8::String>(msg).ToLocalChecked());
}

boolean_t
DTraceConsumer::valid(const dtrace_recdesc_t *rec)
{
	dtrace_actkind_t action = rec->dtrd_action;

	switch (action) {
	case DTRACEACT_DIFEXPR:
	case DTRACEACT_SYM:
	case DTRACEACT_MOD:
	case DTRACEACT_USYM:
	case DTRACEACT_UMOD:
	case DTRACEACT_UADDR:
		return (B_TRUE);

	default:
		return (B_FALSE);
	}
}

v8::Local<v8::Value>
DTraceConsumer::record(const dtrace_recdesc_t *rec, caddr_t addr)
{
	switch (rec->dtrd_action) {
	case DTRACEACT_DIFEXPR:
		switch (rec->dtrd_size) {
		case sizeof (uint64_t):
			return (Nan::New<v8::Number>(*((int64_t *)addr)));

		case sizeof (uint32_t):
			return (Nan::New<v8::Integer>(*((int32_t *)addr)));

		case sizeof (uint16_t):
			return (Nan::New<v8::Integer>(*((uint16_t *)addr)));

		case sizeof (uint8_t):
			return (Nan::New<v8::Integer>(*((uint8_t *)addr)));

		default:
			return	Nan::New<v8::String>((const char *)addr).ToLocalChecked();
		}

	case DTRACEACT_SYM:
	case DTRACEACT_MOD:
	case DTRACEACT_USYM:
	case DTRACEACT_UMOD:
	case DTRACEACT_UADDR:
		dtrace_hdl_t *dtp = dtc_handle;
		char buf[2048], *tick, *plus;

		buf[0] = '\0';

		if (DTRACEACT_CLASS(rec->dtrd_action) == DTRACEACT_KERNEL) {
			uint64_t pc = ((uint64_t *)addr)[0];
			dtrace_addr2str(dtp, pc, buf, sizeof (buf) - 1);
		} else {
			uint64_t pid = ((uint64_t *)addr)[0];
			uint64_t pc = ((uint64_t *)addr)[1];
			dtrace_uaddr2str(dtp, pid, pc, buf, sizeof (buf) - 1);
		}

		if (rec->dtrd_action == DTRACEACT_MOD ||
		    rec->dtrd_action == DTRACEACT_UMOD) {
			/*
			 * If we're looking for the module name, we'll
			 * return everything to the left of the left-most
			 * tick -- or "<undefined>" if there is none.
			 */
			if ((tick = strchr(buf, '`')) == NULL)
				return Nan::New<v8::String>("<unknown>").ToLocalChecked();

			*tick = '\0';
		} else if (rec->dtrd_action == DTRACEACT_SYM ||
		    rec->dtrd_action == DTRACEACT_USYM) {
			/*
			 * If we're looking for the symbol name, we'll
			 * return everything to the left of the right-most
			 * plus sign (if there is one).
			 */
			if ((plus = strrchr(buf, '+')) != NULL)
				*plus = '\0';
		}
		return (Nan::New<v8::String>(buf).ToLocalChecked());
	}

	assert(B_FALSE);

	return Nan::New<v8::Integer>(-1);
}

NAN_METHOD (DTraceConsumer::Strcompile) {

	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;
	dtrace_prog_t *dp;
	dtrace_proginfo_t dinfo;

	if (info.Length() < 1 || !info[0]->IsString()){
		// return (dtc->badarg("expected program"));
	}

	String::Utf8Value program(info[0]->ToString());

	if ((dp = dtrace_program_strcompile(dtp, *program,
	    DTRACE_PROBESPEC_NAME, 0, 0, NULL)) == NULL) {
		// return (dtc->error("couldn't compile '%s': %s\n", *program,
		    // dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

	if (dtrace_program_exec(dtp, dp, &dinfo) == -1) {
		// return (dtc->error("couldn't execute '%s': %s\n", *program,
		    // dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

    info.GetReturnValue().Set(Nan::Undefined());
}


NAN_METHOD (DTraceConsumer::Setopt) {
	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;
	// dtrace_prog_t *dp;
	// dtrace_proginfo_t dinfo;
	int rval;

	if (info.Length() < 1 || !info[0]->IsString()){
		// return (dtc->badarg("expected an option to set"));
		return;
	}

	String::Utf8Value option(info[0]->ToString());

	if (info.Length() >= 2) {
		if (info[1]->IsArray()){
			// return (dtc->badarg("option value can't be an array"));
		}

		if (info[1]->IsObject()){
			// return (dtc->badarg("option value can't be an object"));
		}

		String::Utf8Value optval(info[1]->ToString());
		rval = dtrace_setopt(dtp, *option, *optval);
	} else {
		rval = dtrace_setopt(dtp, *option, NULL);
	}

	if (rval != 0) {
		// return (dtc->error("couldn't set option '%s': %s\n", *option,
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD (DTraceConsumer::Go) {
	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;

	if (dtrace_go(dtp) == -1) {
		// return (dtc->error("couldn't enable tracing: %s\n",
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD (DTraceConsumer::Stop) {

	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;

	if (dtrace_stop(dtp) == -1) { // what do I return
		// return (dtc->error("couldn't disable tracing: %s\n",
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}
    info.GetReturnValue().Set(Nan::Undefined());
}

v8::Local<v8::Object> 
DTraceConsumer::probedesc(const dtrace_probedesc_t *pd)
{
	v8::Local<v8::Object> probe = Nan::New<v8::Object>();
	Nan::Set(probe, Nan::New<v8::String>("provider").ToLocalChecked(),Nan::New<v8::String>(pd->dtpd_provider).ToLocalChecked());
	Nan::Set(probe, Nan::New<v8::String>("module").ToLocalChecked(),Nan::New<v8::String>(pd->dtpd_mod).ToLocalChecked());
	Nan::Set(probe, Nan::New<v8::String>("function").ToLocalChecked(),Nan::New<v8::String>(pd->dtpd_func).ToLocalChecked());
	Nan::Set(probe, Nan::New<v8::String>("name").ToLocalChecked(),Nan::New<v8::String>(pd->dtpd_name).ToLocalChecked());

	return (probe);
}

int
DTraceConsumer::bufhandler(const dtrace_bufdata_t *bufdata, void *arg)
{
	dtrace_probedata_t *data = bufdata->dtbda_probe;
	const dtrace_recdesc_t *rec = bufdata->dtbda_recdesc;
	DTraceConsumer *dtc = (DTraceConsumer *)arg;

	if (rec == NULL || rec->dtrd_action != DTRACEACT_PRINTF)
		return (DTRACE_HANDLE_OK);

	v8::Local<v8::Object> probe = dtc->probedesc(data->dtpda_pdesc);
	v8::Local<v8::Object> record = Nan::New<v8::Object>();
    
    Nan::Set(record, Nan::New<v8::String>("data").ToLocalChecked(),Nan::New<v8::String>(bufdata->dtbda_buffered).ToLocalChecked());
	Local<Value> argv[2] = { probe, record };

	dtc->dtc_callback->Call(dtc->dtc_args->This(), 2, argv);

	return (DTRACE_HANDLE_OK);
}

int
DTraceConsumer::consume(const dtrace_probedata_t *data,
    const dtrace_recdesc_t *rec, void *arg)
{
	DTraceConsumer *dtc = (DTraceConsumer *)arg;
	dtrace_probedesc_t *pd = data->dtpda_pdesc;
	Local<Value> datum;

	v8::Local<v8::Object>  probe = dtc->probedesc(data->dtpda_pdesc);

	if (rec == NULL) {
		Local<Value> info[1] = { probe };
		dtc->dtc_callback->Call(dtc->dtc_args->This(), 1, info);
		return (DTRACE_CONSUME_NEXT);
	}

	if (!dtc->valid(rec)) {
		char errbuf[256];
	
		/*
		 * If this is a printf(), we'll defer to the bufhandler.
		 */
		if (rec->dtrd_action == DTRACEACT_PRINTF)
			return (DTRACE_CONSUME_THIS);

		dtc->dtc_error = dtc->error("unsupported action %s "
		    "in record for %s:%s:%s:%s\n",
		    dtc->action(rec, errbuf, sizeof (errbuf)),
		    pd->dtpd_provider, pd->dtpd_mod,
		    pd->dtpd_func, pd->dtpd_name);	
		return (DTRACE_CONSUME_ABORT);
	}

    v8::Local<v8::Object> record = Nan::New<v8::Object>();
    Nan::Set(record, Nan::New<v8::String>("data").ToLocalChecked(), dtc->record(rec, data->dtpda_data));

	Local<Value> info[2] = { probe, record };

	dtc->dtc_callback->Call(dtc->dtc_args->This(), 2, info);

	return (DTRACE_CONSUME_THIS);
}

NAN_METHOD (DTraceConsumer::Consume) {
	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;
	dtrace_workstatus_t status;

	if (!info[0]->IsFunction()){
		// return (dtc->badarg("expected function as argument"));
		// Nan::ThrowError()
	}

	dtc->dtc_callback = Local<Function>::Cast(info[0]);
	dtc->dtc_args = &info;
	dtc->dtc_error = Nan::Null();

	status = dtrace_work(dtp, NULL, NULL, DTraceConsumer::consume, dtc);

	if (status == -1 && !dtc->dtc_error->IsNull()){
		// return (dtc->dtc_error);
	}

    info.GetReturnValue().Set(Nan::Undefined());
}

/*
 * Caching the quantized ranges improves performance substantially if the
 * aggregations have many disjoing keys.  Note that we only cache a single
 * aggregation variable; programs that have more than one aggregation variable
 * may see significant degradations in performance.  (If this is a common
 * case, this cache should clearly be expanded.)
 */
v8::Local<v8::Array> *
DTraceConsumer::ranges_cached(dtrace_aggvarid_t varid)
{
	if (varid == dtc_ranges_varid)
		return (dtc_ranges);

	return (NULL);
}

v8::Local<v8::Array>  *
DTraceConsumer::ranges_cache(dtrace_aggvarid_t varid, Local<Array> *ranges)
{
	if (dtc_ranges != NULL)
		delete [] dtc_ranges;

	dtc_ranges = ranges;
	dtc_ranges_varid = varid;

	return (ranges);
}

v8::Local<v8::Array> *
DTraceConsumer::ranges_quantize(dtrace_aggvarid_t varid)
{
	int64_t min, max;
	v8::Local<v8::Array> *ranges;
	int i;

	if ((ranges = ranges_cached(varid)) != NULL)
		return (ranges);

	ranges = new Local<Array>[DTRACE_QUANTIZE_NBUCKETS];

	for (i = 0; i < (int)DTRACE_QUANTIZE_NBUCKETS; i++) {
		ranges[i] = Nan::New<v8::Array>(2);

		if (i < (int)DTRACE_QUANTIZE_ZEROBUCKET) {
			/*
			 * If we're less than the zero bucket, our range
			 * extends from negative infinity through to the
			 * beginning of our zeroth bucket.
			 */
			min = i > 0 ? DTRACE_QUANTIZE_BUCKETVAL(i - 1) + 1 :
			    INT64_MIN;
			max = DTRACE_QUANTIZE_BUCKETVAL(i);
		} else if (i == DTRACE_QUANTIZE_ZEROBUCKET) {
			min = max = 0;
		} else {
			min = DTRACE_QUANTIZE_BUCKETVAL(i);
			max = i < (int)DTRACE_QUANTIZE_NBUCKETS - 1 ?
			    DTRACE_QUANTIZE_BUCKETVAL(i + 1) - 1 :
			    INT64_MAX;
		}

		ranges[i]->Set(0, Nan::New<v8::Number>(min));
		ranges[i]->Set(1, Nan::New<v8::Number>(max));
	}

	return (ranges_cache(varid, ranges));
}

v8::Local<v8::Array> *
DTraceConsumer::ranges_lquantize(dtrace_aggvarid_t varid,
    const uint64_t arg)
{
	int64_t min, max;
	v8::Local<v8::Array> *ranges;
	int32_t base;
	uint16_t step, levels;
	int i;

	if ((ranges = ranges_cached(varid)) != NULL)
		return (ranges);

	base = DTRACE_LQUANTIZE_BASE(arg);
	step = DTRACE_LQUANTIZE_STEP(arg);
	levels = DTRACE_LQUANTIZE_LEVELS(arg);

	ranges = new Local<Array>[levels + 2];

	for (i = 0; i <= levels + 1; i++) {
		ranges[i] = Nan::New<v8::Array>(2);

		min = i == 0 ? INT64_MIN : base + ((i - 1) * step);
		max = i > levels ? INT64_MAX : base + (i * step) - 1;

		ranges[i]->Set(0, Nan::New<v8::Number>(min));
		ranges[i]->Set(1, Nan::New<v8::Number>(max));
	}

	return (ranges_cache(varid, ranges));
}

v8::Local<v8::Array> *
DTraceConsumer::ranges_llquantize(dtrace_aggvarid_t varid,
    const uint64_t arg, int nbuckets)
{
	int64_t value = 1, next, step;
	Local<Array> *ranges;
	int bucket = 0, order;
	uint16_t factor, low, high, nsteps;

	if ((ranges = ranges_cached(varid)) != NULL)
		return (ranges);

	factor = DTRACE_LLQUANTIZE_FACTOR(arg);
	low = DTRACE_LLQUANTIZE_LOW(arg);
	high = DTRACE_LLQUANTIZE_HIGH(arg);
	nsteps = DTRACE_LLQUANTIZE_NSTEP(arg);

	ranges = new v8::Local<v8::Array>[nbuckets];

	for (order = 0; order < low; order++)
		value *= factor;

	ranges[bucket] = Nan::New<v8::Array>(2);
	ranges[bucket]->Set(0, Nan::New<v8::Number>(0));
	ranges[bucket]->Set(1, Nan::New<v8::Number>(value - 1));
	bucket++;

	next = value * factor;
	step = next > nsteps ? next / nsteps : 1;

	while (order <= high) {
		ranges[bucket] = Nan::New<v8::Array>(2);
		ranges[bucket]->Set(0, Nan::New<v8::Number>(value));
		ranges[bucket]->Set(1, Nan::New<v8::Number>(value + step - 1));
		bucket++;

		if ((value += step) != next)
			continue;

		next = value * factor;
		step = next > nsteps ? next / nsteps : 1;
		order++;
	}

	ranges[bucket] = Nan::New<v8::Array>(2);
	ranges[bucket]->Set(0, Nan::New<v8::Number>(value));
	ranges[bucket]->Set(1, Nan::New<v8::Number>(INT64_MAX));

	assert(bucket + 1 == nbuckets);

	return (ranges_cache(varid, ranges));
}

int
DTraceConsumer::aggwalk(const dtrace_aggdata_t *agg, void *arg)
{
	DTraceConsumer *dtc = (DTraceConsumer *)arg;
	const dtrace_aggdesc_t *aggdesc = agg->dtada_desc;
	const dtrace_recdesc_t *aggrec;
	Local<Value> val;
	Local<Value> id =  Nan::New<v8::Number>(aggdesc->dtagd_varid);
	Local<Array> key;
	char errbuf[256];
	int i;

	/*
	 * We expect to have both a variable ID and an aggregation value here;
	 * if we have fewer than two records, something is deeply wrong.
	 */
	assert(aggdesc->dtagd_nrecs >= 2);
	key = Nan::New<v8::Array>(aggdesc->dtagd_nrecs - 2);

	for (i = 1; i < aggdesc->dtagd_nrecs - 1; i++) {
		const dtrace_recdesc_t *rec = &aggdesc->dtagd_rec[i];
		caddr_t addr = agg->dtada_data + rec->dtrd_offset;
		Local<Value> datum;

		if (!dtc->valid(rec)) {
			dtc->dtc_error = dtc->error("unsupported action %s "
			    "as key #%d in aggregation \"%s\"\n",
			    dtc->action(rec, errbuf, sizeof (errbuf)), i,
			    aggdesc->dtagd_name);
			return (DTRACE_AGGWALK_ERROR);
		}

		key->Set(i - 1, dtc->record(rec, addr));
	}

	aggrec = &aggdesc->dtagd_rec[aggdesc->dtagd_nrecs - 1];

	switch (aggrec->dtrd_action) {
	case DTRACEAGG_COUNT:
	case DTRACEAGG_MIN:
	case DTRACEAGG_MAX:
	case DTRACEAGG_SUM: {
		caddr_t addr = agg->dtada_data + aggrec->dtrd_offset;

		assert(aggrec->dtrd_size == sizeof (uint64_t));
		val =  Nan::New<v8::Number>(*((int64_t *)addr));
		break;
	}

	case DTRACEAGG_AVG: {
		const int64_t *data = (int64_t *)(agg->dtada_data +
		    aggrec->dtrd_offset);

		assert(aggrec->dtrd_size == sizeof (uint64_t) * 2);
		val =  Nan::New<v8::Number>(data[1] / (double)data[0]);
		break;
	}

	case DTRACEAGG_QUANTIZE: {
		Local<Array> quantize =  Nan::New<v8::Array>();
		const int64_t *data = (int64_t *)(agg->dtada_data +
		    aggrec->dtrd_offset);
		Local<Array> *ranges, datum;
		int i, j = 0;

		ranges = dtc->ranges_quantize(aggdesc->dtagd_varid); 

		for (i = 0; i < (int)DTRACE_QUANTIZE_NBUCKETS; i++) {
			if (!data[i])
				continue;

			datum = Nan::New<v8::Array>(2);
			datum->Set(0, ranges[i]);
			datum->Set(1, Nan::New<v8::Number>(data[i]));

			quantize->Set(j++, datum);
		}

		val = quantize;
		break;
	}

	case DTRACEAGG_LQUANTIZE:
	case DTRACEAGG_LLQUANTIZE: {
		Local<Array> lquantize = Nan::New<v8::Array>();
		const int64_t *data = (int64_t *)(agg->dtada_data +
		    aggrec->dtrd_offset);
		Local<Array> *ranges, datum;
		int i, j = 0;

		uint64_t arg = *data++;
		int levels = (aggrec->dtrd_size / sizeof (uint64_t)) - 1;

		ranges = (aggrec->dtrd_action == DTRACEAGG_LQUANTIZE ?
		    dtc->ranges_lquantize(aggdesc->dtagd_varid, arg) :
		    dtc->ranges_llquantize(aggdesc->dtagd_varid, arg, levels));

		for (i = 0; i < levels; i++) {
			if (!data[i])
				continue;

			datum = Nan::New<v8::Array>(2);
			datum->Set(0, ranges[i]);
			datum->Set(1, Nan::New<v8::Number>(data[i]));

			lquantize->Set(j++, datum);
		}

		val = lquantize;
		break;
	}

	default:
		dtc->dtc_error = dtc->error("unsupported aggregating action "
		    " %s in aggregation \"%s\"\n", dtc->action(aggrec, errbuf,
		    sizeof (errbuf)), aggdesc->dtagd_name);
		return (DTRACE_AGGWALK_ERROR);
	}

	Local<Value> argv[3] = { id, key, val };
	dtc->dtc_callback->Call(dtc->dtc_args->This(), 3, argv);

	return (DTRACE_AGGWALK_REMOVE);
}

NAN_METHOD (DTraceConsumer::Aggclear) {

	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;

	if (dtrace_status(dtp) == -1) {
		// return (dtc->error("couldn't get status: %s\n",
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

	dtrace_aggregate_clear(dtp);
    info.GetReturnValue().Set(Nan::Undefined());
}


NAN_METHOD (DTraceConsumer::Aggwalk) {

	DTraceConsumer *dtc =  Nan::ObjectWrap::Unwrap<DTraceConsumer>(info.Holder());
	dtrace_hdl_t *dtp = dtc->dtc_handle;
	int rval;

	if (!info[0]->IsFunction()){
		// return (dtc->badarg("expected function as argument"));
	}

	dtc->dtc_callback = Local<Function>::Cast(info[0]);
	dtc->dtc_args = &info;
	dtc->dtc_error = Nan::Null();

	if (dtrace_status(dtp) == -1) {
		// return (dtc->error("couldn't get status: %s\n",
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

	if (dtrace_aggregate_snap(dtp) == -1) {
		// return (dtc->error("couldn't snap aggregate: %s\n",
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

	rval = dtrace_aggregate_walk(dtp, DTraceConsumer::aggwalk, dtc);

	/*
	 * Flush the ranges cache; the ranges will go out of scope when the
	 * destructor for our HandleScope is called, and we cannot be left
	 * holding references.
	 */
	dtc->ranges_cache(DTRACE_AGGVARIDNONE, NULL);

	if (rval == -1) {
		if (!dtc->dtc_error->IsNull()){
			// return (dtc->dtc_error);
		}

		// return (dtc->error("couldn't walk aggregate: %s\n",
		//     dtrace_errmsg(dtp, dtrace_errno(dtp))));
	}

    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD (DTraceConsumer::Aggmin) {
    info.GetReturnValue().Set(Nan::New<v8::Number>(INT64_MIN));
}

NAN_METHOD (DTraceConsumer::Aggmax) {
    info.GetReturnValue().Set(Nan::New<v8::Number>(INT64_MAX));
}

NAN_METHOD (DTraceConsumer::Version) {
    info.GetReturnValue().Set(Nan::New<v8::String>(_dtrace_version).ToLocalChecked());
}

// extern "C" void
// init (Handle<Object> target) 
// {
// 	DTraceConsumer::Initialize(target);
// }

NODE_MODULE(dtrace, DTraceConsumer::Init);
