cwlVersion: v1.2
class: CommandLineTool
baseCommand: [FishnetVectorFileSplitter]
hints:
  DockerRequirement:
    dockerPull: ghcr.io/descartesresearch/fishnet-apps:2.0.0
requirements:
  InlineJavascriptRequirement: {}
  ResourceRequirement:
    coresMin: 1
    ramMin: 1000
inputs:
  shapefile:
    type: File
    secondaryFiles: [^.shx, ^.dbf, ^.prj, ^.cpg?, ^.qpj?]
    # format: SHP
    inputBinding:
        prefix: --input 
  outputDir:
    type: string
    default: "./"
    inputBinding:
        prefix: -o
    doc: "Output directory"
  depth:
    type: int
    inputBinding:
        prefix: --depth
  xOffset:
    type: int
    default: 0
    inputBinding:
        prefix: -x
    doc: "X offset for the naming of the output tiles"
  yOffset:
    type: int
    default: 0
    inputBinding:
        prefix: -y
    doc: "Y offset for the naming of the output tiles"
outputs:
  split_shapefiles:
    type: File[]
    # format: SHP
    secondaryFiles: [^.shx, ^.dbf, ^.prj, ^.cpg?, ^.qpj?]
    outputBinding:
        glob: "$(inputs.shapefile.nameroot)*.shp"
    doc: "Split output files"
stdout: SPLIT_$(inputs.shapefile.nameroot)_stdout.log
stderr: SPLIT_$(inputs.shapefile.nameroot)_stderr.log
    