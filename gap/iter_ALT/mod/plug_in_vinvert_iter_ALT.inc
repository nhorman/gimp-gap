/* ----------------------------------------------------------------------
 * p_plug_in_vinvert_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_vinvert_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_vinvert_Vals 
    {
      gint dummy;
    } t_plug_in_vinvert_Vals; 

    t_plug_in_vinvert_Vals  buf, *buf_from, *buf_to; 


   /* this iterator is a dummy, and does not iterate because the plug_in_vinvert
    * does not have LAST_VALUES buffer. but also does not require any parameter to run.
    * the dummy iterator was added to get plug_in_vinvert into the GAP DBbrowser preselection list
    * (plug-ins without any iterator do not appear in GAP DBBrowser any more, to filter out
    * a lot of plug-ins that do not make sense with Filter All Layers)
    */
    
   /*
    *if(len_struct != sizeof(t_plug_in_vinvert_Vals)) 
    *{
    *  fprintf(stderr, "ERROR: p_plug_in_vinvert_iter_ALT  stored Data missmatch in size %d != %d\n",   
    *                   (int)len_struct, sizeof(t_plug_in_vinvert_Vals) ); 
    *  return -1;
    *}
    */
    gimp_get_data("plug_in_vinvert_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_vinvert_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_vinvert_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_vinvert_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));


    gimp_set_data("plug_in_vinvert", &buf, sizeof(buf)); 

    return 0; /* OK */
}
