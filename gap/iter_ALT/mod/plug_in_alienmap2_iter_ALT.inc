/* ----------------------------------------------------------------------
 * p_plug_in_alienmap2_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_alienmap2_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef enum
    {
      RGB_MODEL = 0,
      HSL_MODEL = 1
    } t_plug_in_alienmap2_ColorModel;

    typedef struct t_plug_in_alienmap2_Vals 
    {
      gdouble    redfrequency;
      gdouble    redangle;
      gdouble    greenfrequency;
      gdouble    greenangle;
      gdouble    bluefrequency;
      gdouble    blueangle;
      t_plug_in_alienmap2_ColorModel colormodel;
      gboolean   redmode;
      gboolean   greenmode;
      gboolean   bluemode;
    } t_plug_in_alienmap2_Vals; 

    t_plug_in_alienmap2_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_alienmap2_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_alienmap2_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_alienmap2_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug-in-alienmap2-ITER-FROM", g_plugin_data_from); 
    gimp_get_data("plug-in-alienmap2-ITER-TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_alienmap2_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_alienmap2_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.redfrequency, buf_from->redfrequency, buf_to->redfrequency, total_steps, current_step);
    p_delta_gdouble(&buf.redangle, buf_from->redangle, buf_to->redangle, total_steps, current_step);
    p_delta_gdouble(&buf.greenfrequency, buf_from->greenfrequency, buf_to->greenfrequency, total_steps, current_step);
    p_delta_gdouble(&buf.greenangle, buf_from->greenangle, buf_to->greenangle, total_steps, current_step);
    p_delta_gdouble(&buf.bluefrequency, buf_from->bluefrequency, buf_to->bluefrequency, total_steps, current_step);
    p_delta_gdouble(&buf.blueangle, buf_from->blueangle, buf_to->blueangle, total_steps, current_step);

    gimp_set_data("plug-in-alienmap2", &buf, sizeof(buf)); 

    return 0; /* OK */
}
