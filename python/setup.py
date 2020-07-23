from setuptools import setup

setup(name="pyimplot",
      version="0.1",
      description="Python bindings for implot with similarity to matplotlib.",
      url="https://github.com/joruof/pyimplot",
      author="Jona Ruof",
      author_email="jona.ruof@uni-ulm.de",
      license="MIT",
      zip_safe=True,
      packages=[''],
      package_dir={'': 'build'},
      package_data={'': ["pyimplot.cpython-38-x86_64-linux-gnu.so"]},
    )

