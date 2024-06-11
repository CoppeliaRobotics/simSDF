sim = require 'sim'

function importSDF()
    local success, err = pcall(function()
        simSDF.import(options.fileName, options)
    end)
    if err then
        simUI.msgBox(simUI.msgbox_type.info, simUI.msgbox_buttons.ok, 'Error', 'Error: ' .. err)
    end
    closeDialog()
end

function closeDialog()
    if ui then
        simUI.destroy(ui)
        ui = nil
    end
    done = true
end

function updateOptions(ui, id, val)
    local function val2bool(v)
        if v == 0 then
            return false
        else
            return true
        end
    end
    if optionsInfo[id] then options[optionsInfo[id].key] = val2bool(val) end
end

function sysCall_info()
    return {autoStart = false, menu = 'Importers\nSDF importer...'}
end

function sysCall_init()
    simSDF = require 'simSDF'
    simUI = require 'simUI'

    closeDialog()

    optionsInfo = {
        [10] = {name = 'Ignore missing values', key = 'ignoreMissingValues'},
        [20] = {name = 'Hide collision links', key = 'hideCollisionLinks'},
        [30] = {name = 'Hide joints', key = 'hideJoints'},
        [40] = {name = 'Convex decompose', key = 'convexDecompose'},
        [50] = {name = 'Show convex decomp. dlg', key = 'showConvexDecompositionDlg'},
        [60] = {name = 'Create visual if none', key = 'createVisualIfNone'},
        [70] = {name = 'Center model', key = 'centerModel'},
        [80] = {name = 'Prepare model', key = 'prepareModel'},
        [90] = {name = 'No self-collision', key = 'noSelfCollision'},
        [100] = {name = 'Position ctrl', key = 'positionCtrl'},
    }

    options = {
        ignoreMissingValues = false,
        hideCollisionLinks = true,
        hideJoints = true,
        convexDecompose = true,
        showConvexDecompositionDlg = false,
        createVisualIfNone = true,
        centerModel = true,
        prepareModel = true,
        noSelfCollision = true,
        positionCtrl = true,
    }

    local scenePath = sim.getStringParam(sim.stringparam_scene_path)
    local fileNames = simUI.fileDialog(simUI.filedialog_type.load, 'Import SDF...', scenePath, '', 'SDF file', 'sdf;world')

    if #fileNames > 0 then
        done = false
        options.fileName = fileNames[1]
        local xml = '<ui modal="true" layout="vbox" title="Importing ' .. options.fileName ..
                        '..." closeable="true" on-close="closeDialog">\n'
        for id, o in pairs(optionsInfo) do
            xml = xml .. '<checkbox id="' .. id .. '" checked="' ..
                      (options[o.key] and 'true' or 'false') .. '" text="' .. o.name ..
                      '" on-change="updateOptions" />\n'
        end
        xml = xml .. '<button text="Import" on-click="importSDF" />\n'
        xml = xml .. '</ui>'
        ui = simUI.create(xml)
    end
end

function sysCall_nonSimulation()
    if done then return {cmd = 'cleanup'} end
end

function sysCall_cleanup()
    closeDialog()
end
