#ifndef AUX_MICRO_H_INCLUDED
#define AUX_MICRO_H_INCLUDED

extern void
auxm_send_cmd(
    int	    cmd,
    int	    len,
    char*   data);

extern int auxm_serial_dump(void);

extern int machine_has_auxm(void);
extern int auxm_init(void);

extern void auxm_serial_check(void);

extern void check_3800_func_buttons(void);

extern void auxm_fini_serial(void);

extern void hilite_button(int which);

extern void exec_button(int which);
    
extern void set_last_buttonpress(char which);
extern int get_last_buttonpress(void);

extern void set_exec_buttons_automatically(char c);
extern char get_exec_buttons_automatically(void);

#endif
