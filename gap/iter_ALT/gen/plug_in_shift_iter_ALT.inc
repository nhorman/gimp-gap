/* ----------------------------------------------------------------------
 * p_plug_in_shift_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_shift_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_shift_Vals 
    {
      gint      shift_amount;
      gint      orientation;
    } t_plug_in_shift_Vals; 

    t_plug_in_shift_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_shift_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_shift_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_shift_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-shift-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-shift-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_shift_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_shift_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.shift_amount, buf_from->shift_amount, buf_to->shift_amount, total_steps, current_step);
    p_delta_gint(&buf.orientation, buf_from->orientation, buf_to->orientation, total_steps, current_step);

    gimp_set_data("plug-in-shift", &buf, sizeof(buf)); 

    return 0; /* OK */
}
