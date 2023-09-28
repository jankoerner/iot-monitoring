FROM python:3.11.5-bullseye

ENV PYTHONUNBUFFERED 1

RUN apt-get update && apt-get install -y \
    netcat \
    vim \
    nano \
    curl

WORKDIR /core

COPY . .

#RUN pip install -r requirements.txt

EXPOSE 12001:12001

#ENTRYPOINT ["tail", "-f", "/dev/null"]
ENTRYPOINT ["/usr/local/bin/python", "/core/sink.py"]