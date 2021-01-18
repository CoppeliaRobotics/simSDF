function importSDF()
    local success,err=pcall(function() simSDF.import(options.fileName,options) end)
    if err then
        simUI.msgBox(simUI.msgbox_type.info,simUI.msgbox_buttons.ok,'Error','Error: '..err)
    end
    closeDialog()
end

function closeDialog()
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

function checkbox(id,text,varname)
    return '<checkbox id="'..id..'" checked="'..bool2string(options[varname])..'" text="'..text..'" on-change="updateOptions" />\n'
end

function button(text,fn)
    return '<button text="'..text..'" on-click="'..fn..'" />\n'
end

function sysCall_init()
    if ui then
        closeDialog()
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
            '<ui modal="true" layout="vbox" title="Importing '..fileName..'..." closeable="true" on-close="closeDialog">\n'..
            checkbox("10","Ignore missing values",'ignoreMissingValues')..
            checkbox("20","Hide collision links",'hideCollisionLinks')..
            checkbox("30","Hide joints",'hideJoints')..
            checkbox("40","Convex decompose",'convexDecompose')..
            checkbox("50","Show convex decomp. dlg",'showConvexDecompositionDlg')..
            checkbox("60","Create visual if none",'createVisualIfNone')..
            checkbox("70","Center model",'centerModel')..
            checkbox("80","Prepare model",'prepareModel')..
            checkbox("90","No self-collision",'noSelfCollision')..
            checkbox("100","Position ctrl",'positionCtrl')..
            button('Import','importSDF')..
            '</ui>'
        )
    end

    return sim.syscb_cleanup
end
