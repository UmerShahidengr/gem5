import resource
from gem5.resources.resource import Resource

resource = Resource("riscv-disk-img")
print(f"The resources ia at {resource.get_local_path()}")
