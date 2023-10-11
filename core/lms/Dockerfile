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

EXPOSE 12004:12004

#ENTRYPOINT ["tail", "-f", "/dev/null"]
ENTRYPOINT ["/usr/local/bin/python", "/core/lms.py"]