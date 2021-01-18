function importSDF()
    simSDF.import(options.fileName,options)
    simUI.destroy(ui)
    ui=nil
end

function val2bool(v)
    if v==0 then return false else return true end
end

function updateOptions(ui,id,val)
    if id==10 then
        options.ignoreMissingValues=val2bool(val)
    elseif id==20 then
        options.hideCollisionLinks=val2bool(val)
    elseif id==30 then
        options.hideJoints=val2bool(val)
    elseif id==40 then
        options.convexDecompose=val2bool(val)
    elseif id==50 then
        options.showConvexDecompositionDlg=val2bool(val)
    elseif id==60 then
        options.createVisualIfNone=val2bool(val)
    elseif id==70 then
        options.centerModel=val2bool(val)
    elseif id==80 then
        options.prepareModel=val2bool(val)
    elseif id==90 then
        options.noSelfCollision=val2bool(val)
    elseif id==100 then
        options.positionCtrl=val2bool(val)
    end
end

function bool2string(b)
    if b then return "true" else return "false" end
end

function label(text)
    return '<label text="'..text..'" />\n'
end

function checkbox(id,varname)
    return '<checkbox id="'..id..'" checked="'..bool2string(options[varname])..'" text="" on-change="updateOptions" />\n'
end

function button(text,fn)
    return '<button text="'..text..'" on-click="'..fn..'" />\n'
end

function sysCall_init()
    if ui then
        simUI.destroy(ui)
        ui=nil
    end

    options={
        ignoreMissingValues=false,
        hideCollisionLinks=true,
        hideJoints=true,
        convexDecompose=true,
        showConvexDecompositionDlg=false,
        createVisualIfNone=true,
        centerModel=true,
        prepareModel=true,
        noSelfCollision=true,
        positionCtrl=true,
    }

    local scenePath=sim.getStringParameter(sim.stringparam_scene_path)
    local fileName=sim.fileDialog(sim.filedlg_type_load,'Import SDF...',scenePath,'','SDF file','sdf')

    if fileName then
        options.fileName=fileName
        ui=simUI.create(
            '<ui modal="true" layout="form" title="Importing '..fileName..'...">\n'..
            label("Ignore missing values")..checkbox("10",'ignoreMissingValues')..
            label("Hide collision links")..checkbox("20",'hideCollisionLinks')..
            label("Hide joints")..checkbox("30",'hideJoints')..
            label("Convex decompose")..checkbox("40",'convexDecompose')..
            label("Show convex decomp. dlg")..checkbox("50",'showConvexDecompositionDlg')..
            label("Create visual if none")..checkbox("60",'createVisualIfNone')..
            label("Center model")..checkbox("70",'centerModel')..
            label("Prepare model")..checkbox("80",'prepareModel')..
            label("No self-collision")..checkbox("90",'noSelfCollision')..
            label("Position ctrl")..checkbox("100",'positionCtrl')..
            label("")..button('Import','importSDF')..
            '</ui>'
        )
    end

    return sim.syscb_cleanup
end
