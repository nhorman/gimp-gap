/* ----------------------------------------------------------------------
 * p_plug_in_CML_explorer_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_CML_explorer_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{

    typedef struct t_plug_in_CML_explorer_Vals 
    {
     t_CML_PARAM hue;
     t_CML_PARAM sat;
     t_CML_PARAM val;
     gint      initial_value;
     gint      scale;
     gint      start_offset;
     gint      seed;
     gchar     last_file_name[256];
    } t_plug_in_CML_explorer_Vals; 

    t_plug_in_CML_explorer_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_CML_explorer_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_CML_explorer_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_CML_explorer_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_CML_explorer_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_CML_explorer_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_CML_explorer_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_CML_explorer_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_CML_PARAM(&buf.hue, &buf_from->hue, &buf_to->hue, total_steps, current_step);
    p_delta_CML_PARAM(&buf.sat, &buf_from->sat, &buf_to->sat, total_steps, current_step);
    p_delta_CML_PARAM(&buf.val, &buf_from->val, &buf_to->val, total_steps, current_step);

    p_delta_gint(&buf.initial_value, buf_from->initial_value, buf_to->initial_value, total_steps, current_step);
    p_delta_gint(&buf.scale, buf_from->scale, buf_to->scale, total_steps, current_step);
    p_delta_gint(&buf.start_offset, buf_from->start_offset, buf_to->start_offset, total_steps, current_step);
    p_delta_gint(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);

    gimp_set_data("plug_in_CML_explorer", &buf, sizeof(buf)); 

    return 0; /* OK */
}
