
README_onionskin

REQUIRED:
  gimp 1.3.14

INSTALLATION:
  # the onionskin module was integrated into the gimp-gap basepackage
  # and need no seperate installation.

After successful installation of the gimp-gap basepackage
the gimp Video Menu has a submenu named "Onionskin"

<Image>/Video/Onionskin/Configuration  
<Image>/Video/Onionskin/Create or Replace
<Image>/Video/Onionskin/Delete
<Image>/Video/Onionskin/Toggle Visibility


DESCRIPTION:

  This Module handles Onionskin Layers in the GIMP Video Menu.
  Onionskin Layer(s) usually do show previous (or next) frame(s)
  of the video in the current frame.
  

  In this implementation onionskin layers are copies of
  previous or next frames of the video. By default
  they are built by merging the visible layers of the
  previous frame but without the BG-Layer 
  (and without other onionlayers).
  Onionskin layers are usually placed above the backroundlayer
  in the layerstack of the current frame.  
  
  Onion Layers are normally used for display of the previous
  scene when painting moving characters.
  They behave like a normal layer, but you can toggle
  visibility or delete onionskin layers
  with Functions provided by this module.
    


  Configuration:
    In the onionskin configuration dialog window you can
    - edit the settings for creation of onionskin layers
    - apply the settings to a range of frames
    - delete onionskin layers in selected range of frames.
    
    APPLY:
    The apply button creates onion layers in the selected range
    of frames and sets the values for this gimp session.

    DELETE:
    The delete button removes onion layers in the selected range
    of frames and sets the values for this gimp session.
    

    SET:
    This button sets the values for the current
    gimp session (without changing any frame).
    
    
    DEFAULT:
    Load hardcoded defaults (does not set anything)
    
    
    
    Onionskin Layers:
         Defines how many onionlayers will be created in
         the current image.
         
    Frame Reference:
         Stepsize to reference the source framenumber
         from where we copy the onionlayer.
         Examples:
         -1  .. copy 1.st onionlayer from current framenumber -1
                copy 2.nd onionlayer from current framenumber -2
                
         +2  .. copy 1.st onionlayer from current framenumber +2
                copy 2.nd onionlayer from current framenumber +4

    Cyclic (Togglebutton):
         Define fold back behavior when referencing previous
         or next frames:
         ON:  last frame has frame 0 as next frame.
         OFF: last frame has NO next frame.

    Stackposition:
         Position where to place onionlayer(s) in the layerstack.
    From TOP (Togglebutton):
         ON: Stackposition is counted from top (0 is on top)
         OFF: Stackposition is counted from bottom (0 is on bottom)

    Opacity:
         The 1.st onionlayer is created with this
         opacity seting (100 is full opaque, 0 is full transparent)

    Opacity Delta:
         100 : The 2.nd onionlayer is created with same
               opacity as the 1.st onionlayer.
         80  : The 2.nd onionlayer is created with 
               80% of the opacity of the 1.st onionlayer,
               The 3.rd with 80% of the 2.nd  and so on...
 
    Options how to merge the previous Layer into one
    Onionlayer:
  
      Restrict by Layernames:  Default '*'  all Names.
         Here you can specify a layernamepattern.
         only layers with matching names are included
         in the onionlayer.
      
      Ignore BottomLayer(s):   Default 1
         Here you can exclude the background layer (1)
         or n layers counted from the bottom of the
         layerstack from the merged copy of the frame
         that makes up the onionlayer.
        
  
  
  The other functions in the Video/OnionskinLayer menu
  do not open dialog windows. 
  They are intended for quick use, and it is recommanded
  to assign key-shortcuts if you want to use them
  frequently.
  
  
  Create or Update Onion Layers:
       Create onionlayers in the current frame,
       according to the configurated settings.
       (see above)
  
  Toggle Visibility:
       Turn visibilty on (or off) for all onionlayers.

  Delete Onion Layers:
       Remove all onionlayers from the current frame.
 
       Note: if you made a copy of an onionlayer manually
             the copy should not be deleted.
             If you have gimp-1.2.2 the copy will be deleted too,
             because there is a bug in the gimp video base functions.
  
