from PIL import Image
from argparse import ArgumentParser

parser = ArgumentParser("png2ico")
parser.add_argument("input_path")
parser.add_argument("output_path")
#parser.add_argument("--size", "-s", type=int, help="Size in pixels of the (square) output image")
args = parser.parse_args()

Image.open(args.input_path).save(args.output_path, format="ICO", sizes=[(32, 32), (64, 64), (128, 128), (256, 256)])