/* ----------------------------------------------------------------------
 * p_plug_in_lighting_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_lighting_iter_ALT(GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
#define LIGHTING_NUM_LIGHTS      6
    typedef struct t_plug_in_lighting_Vals 
    {
      gint32    drawable_id;
      gint32    bumpdrawable;
      gint32    envdrawable;

      /* Render variables */
      /* ================ */
  
      GimpVector3       viewpoint;
      GimpVector3       planenormal;
      t_LightingLightSettings    lightsource[LIGHTING_NUM_LIGHTS];
      t_LightingMaterialSettings material;
      t_LightingMaterialSettings refmaterial;

      gdouble pixel_treshold;
      gdouble bumpmax;
      gdouble bumpmin;

      gint    max_depth;
      gint    bumpmaptype;

      /* Flags */
      /* ===== */

      gint antialiasing;
      gint create_new_image;
      gint transparent_background;
      gint bump_mapped;
      gint env_mapped;
      gint ref_mapped;
      gint bumpstretch;
      gint previewquality;

      gboolean symbols;
      gboolean interactive_preview;
 

      /* Misc */
      /* ==== */
      gboolean     update_enabled;
      gint         light_selected;
      gboolean     light_isolated;
      gdouble      preview_zoom_factor;
      
    } t_plug_in_lighting_Vals; 

    t_plug_in_lighting_Vals  buf, buf_from, buf_to;
    gint ii;

    if(len_struct != sizeof(t_plug_in_lighting_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_lighting_iter  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_lighting_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_lighting_ITER_FROM", &buf_from); 
    gimp_get_data("plug_in_lighting_ITER_TO",   &buf_to); 
    memcpy(&buf, &buf_from, sizeof(buf));


    p_delta_GimpVector3(&buf.viewpoint, &buf_from.viewpoint, &buf_to.viewpoint, total_steps, current_step);
    p_delta_GimpVector3(&buf.planenormal, &buf_from.planenormal, &buf_to.planenormal, total_steps, current_step);

    for(ii=0; ii < LIGHTING_NUM_LIGHTS; ii++)
    {
      p_delta_LightingLightSettings(&buf.lightsource[ii], &buf_from.lightsource[ii], &buf_to.lightsource[ii], total_steps, current_step);
    }
    p_delta_LightingMaterialSettings(&buf.material, &buf_from.material, &buf_to.material, total_steps, current_step);
    p_delta_LightingMaterialSettings(&buf.refmaterial, &buf_from.refmaterial, &buf_to.refmaterial, total_steps, current_step);

    
    p_delta_gdouble(&buf.pixel_treshold, buf_from.pixel_treshold, buf_to.pixel_treshold, total_steps, current_step);
    p_delta_gdouble(&buf.bumpmax, buf_from.bumpmax, buf_to.bumpmax, total_steps, current_step);
    p_delta_gdouble(&buf.bumpmin, buf_from.bumpmin, buf_to.bumpmin, total_steps, current_step);
    p_delta_gint(&buf.max_depth, buf_from.max_depth, buf_to.max_depth, total_steps, current_step);
    p_delta_gint(&buf.bumpmaptype, buf_from.bumpmaptype, buf_to.bumpmaptype, total_steps, current_step);

    p_delta_gint(&buf.antialiasing, buf_from.antialiasing, buf_to.antialiasing, total_steps, current_step);
    p_delta_gint(&buf.transparent_background, buf_from.transparent_background, buf_to.transparent_background, total_steps, current_step);
    p_delta_gint(&buf.bump_mapped, buf_from.bump_mapped, buf_to.bump_mapped, total_steps, current_step);
    p_delta_gint(&buf.ref_mapped, buf_from.ref_mapped, buf_to.ref_mapped, total_steps, current_step);
    p_delta_gint(&buf.bumpstretch, buf_from.bumpstretch, buf_to.bumpstretch, total_steps, current_step);
    p_delta_gint(&buf.previewquality, buf_from.previewquality, buf_to.previewquality, total_steps, current_step);

    p_delta_gint(&buf.light_selected, buf_from.light_selected, buf_to.light_selected, total_steps, current_step);

    gimp_set_data("plug_in_lighting", &buf, sizeof(buf)); 

    return 0; /* OK */
}
