import json
import string
import xml.etree.ElementTree as ET
from argparse import ArgumentParser

import jenkins


data_type_sizes ={
    "Float3":       12,
    "D3dcolor":     4,
    "Float2":       8,
    "Float4":       16,
    "ubyte4n":      4,
    "Float16_2":    4,
    "float16_2":    4,
    "Short2":       4,
    "Float1":       4,
    "Short4":       8
}

def main():
    parser = ArgumentParser(description="Converts a materials xml file to json for use in dme loading")
    parser.add_argument("input", help="Input file name of xml data")
    parser.add_argument("output", help="Output file name")
    args = parser.parse_args()

    tree = ET.parse(args.input)
    root = tree.getroot()
    input_layouts = root[0]
    parameter_groups = root[1]
    material_definitions = root[2]

    layout_dict = {}
    param_group_dict = {}
    definition_dict = {}

    for matdef in material_definitions:
        name = matdef.get("Name")
        if not name:
            continue
        properties = []
        draw_styles = []
        for child in matdef:
            if child.get("Name") == "DrawStyles":
                for draw_style_node in child:
                    draw_style = {
                        key[0].lower() + key[1:]:value 
                            for key, value in draw_style_node.attrib.items() 
                                if key != "Class"
                    }
                    draw_style["hash"] = jenkins.oaat(draw_style["name"].encode("utf-8"))
                    draw_styles.append(draw_style)
            elif child.get("Name") == "Properties":
                for properties_node in child:
                    if not properties_node.get("Class") or "Parameter" not in properties_node.get("Class"):
                        continue
                    property = {
                        key[0].lower() + key[1:]:value 
                            for key, value in properties_node.attrib.items() 
                                if key != "Class"
                    }
                    property["hash"] = jenkins.oaat(property["name"].encode("utf-8"))
                    properties.append(property)
        
        definition_dict[jenkins.oaat(name.encode("utf-8"))] = {
            "name": name,
            "hash": jenkins.oaat(name.encode("utf-8")),
            "properties": properties,
            "drawStyles": draw_styles,
        }
    
    for input_layout in input_layouts:
        name = input_layout.get("Name")
        entries = []
        sizes = {}
        for entry_node in input_layout[0]:
            entry = {}
            for key, value in entry_node.attrib.items():
                if key == "Class":
                    continue
                if key == "Type":
                    if entry_node.attrib["Stream"] not in sizes:
                        sizes[entry_node.attrib["Stream"]] = 0
                    sizes[entry_node.attrib["Stream"]] += data_type_sizes[value]
                entry[key[0].lower() + key[1:]] = value if not value.isnumeric() else int(value)
            entries.append(entry)
        layout_dict[name] = {
            "name": name,
            "sizes": sizes,
            "hash": jenkins.oaat(name.encode("utf-8")),
            "entries": entries
        }
    
    for param_group in parameter_groups:
        name = param_group.get("Name")
        parameters = [{key[0].lower() + key[1:]:value for key, value in param_node.attrib.items()} for param_node in param_group[0]]
        param_group_dict[name] = {
            "name": name,
            "hash": jenkins.oaat(name.encode("utf-8")),
            "parameters": parameters
        }
    
    materials = {
        "inputLayouts": layout_dict,
        "parameterGroups": param_group_dict,
        "materialDefinitions": definition_dict
    }

    with open(args.output, "w") as output:
        json.dump(materials, output, indent=4)



if __name__ == "__main__":
    main()