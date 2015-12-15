// libxxx.cpp 

extern "C" {

    typedef void (*t_get_crypto_key_cb)(void * context, unsigned int status);

    t_get_crypto_key_cb global_cb_func = nullptr;
    void * global_context = nullptr;

    int get_crypto_key_cb_set(t_get_crypto_key_cb cb_func, void * context)
    {
        global_cb_func = cb_func;
        global_context = context;
        return 1;
    }

    void run()
    {
        if (global_context && global_cb_func){
            global_cb_func(global_context, 10);
        }
    }
}
