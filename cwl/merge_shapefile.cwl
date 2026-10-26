cwlVersion: v1.2
class: CommandLineTool
baseCommand: [FishnetVectorFileMerger]
hints:
  DockerRequirement:
    dockerPull: ghcr.io/descartesresearch/fishnet-apps:2.0.1
requirements:
  InlineJavascriptRequirement: {}
  ResourceRequirement:
    coresMin: 1
    ramMin: 1000
inputs:
  shpFiles:
    type: File[]
    secondaryFiles: [^.shx, ^.dbf, ^.prj, ^.cpg?, ^.qpj?]
    # format: SHP
    inputBinding:
        prefix: -i
    doc: "List of input shapefiles, with their required secondary files (.dbf, .shx, .prj)"
  outputPath:
    type: string
    inputBinding:
        position: 2
        prefix: -o 
        valueFrom: $(self+".shp")
    doc: "Output filename for result (Shapefile)"  
outputs:
  mergedOutput:
    type: File
    secondaryFiles: [^.shx, ^.dbf, ^.prj, ^.cpg?, ^.qpj?]
    # format: SHP
    outputBinding:
        glob: "$(inputs.outputPath).shp"
    doc: "Merged output file"
stdout: MERGE_stdout.log
stderr: MERGE_stderr.log